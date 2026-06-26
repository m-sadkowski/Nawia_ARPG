#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "CombatTelemetryServer.h"

#include <json.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

namespace Nawia::Game {

	namespace {
		constexpr auto WORKER_SLEEP = std::chrono::milliseconds(16);

		[[nodiscard]] const char* toString(const Entity::EntityType type) {
			switch (type) {
				case Entity::EntityType::None:
					return "None";
				case Entity::EntityType::Player:
					return "Player";
				case Entity::EntityType::Enemy:
					return "Enemy";
				case Entity::EntityType::Ally:
					return "Ally";
				case Entity::EntityType::NPCActor:
					return "NPCActor";
				case Entity::EntityType::NPCStatic:
					return "NPCStatic";
				case Entity::EntityType::Projectile:
					return "Projectile";
				case Entity::EntityType::Trigger:
					return "Trigger";
				case Entity::EntityType::Chest:
					return "Chest";
				case Entity::EntityType::Item:
					return "Item";
			}

			return "Unknown";
		}

		[[nodiscard]] const char* toString(const Entity::Faction faction) {
			switch (faction) {
				case Entity::Faction::Player:
					return "Player";
				case Entity::Faction::Enemy:
					return "Enemy";
				case Entity::Faction::Neutral:
					return "Neutral";
				case Entity::Faction::Ally:
					return "Ally";
				case Entity::Faction::None:
					return "None";
			}

			return "Unknown";
		}

		[[nodiscard]] const char* toString(const Entity::AbilityTargetType target_type) {
			switch (target_type) {
				case Entity::AbilityTargetType::POINT:
					return "POINT";
				case Entity::AbilityTargetType::UNIT:
					return "UNIT";
				case Entity::AbilityTargetType::SELF:
					return "SELF";
			}

			return "UNKNOWN";
		}

		[[nodiscard]] nlohmann::json vectorToJson(const Vector2 value) {
			return {
				{"x", value.x},
				{"y", value.y}
			};
		}

		[[nodiscard]] nlohmann::json entityRefToJson(const CombatEntityRef& ref) {
			return {
				{"valid", ref.valid},
				{"runtime_id", static_cast<std::uint64_t>(ref.runtime_id)},
				{"name", ref.name},
				{"entity_type", toString(ref.type)},
				{"faction", toString(ref.faction)},
				{"position", vectorToJson(ref.position)},
				{"hp", ref.hp},
				{"max_hp", ref.max_hp}
			};
		}

		[[nodiscard]] nlohmann::json agentEntityToJson(const AgentEntitySnapshot& snapshot) {
			return {
				{"valid", snapshot.valid},
				{"runtime_id", static_cast<std::uint64_t>(snapshot.runtime_id)},
				{"name", snapshot.name},
				{"entity_type", toString(snapshot.type)},
				{"faction", toString(snapshot.faction)},
				{"position", vectorToJson(snapshot.position)},
				{"velocity", vectorToJson(snapshot.velocity)},
				{"hp", snapshot.hp},
				{"max_hp", snapshot.max_hp},
				{"hp_ratio", snapshot.hp_ratio},
				{"alive", snapshot.alive},
				{"dying", snapshot.dying},
				{"dormant", snapshot.dormant},
				{"visible", snapshot.visible},
				{"interactable", snapshot.interactable},
				{"interaction_available", snapshot.interaction_available},
				{"interaction_range", snapshot.interaction_range},
				{"interaction_state", snapshot.interaction_state},
				{"moving", snapshot.moving},
				{"rooted", snapshot.rooted},
				{"poisoned", snapshot.poisoned},
				{"root_remaining", snapshot.root_remaining},
				{"poison_remaining", snapshot.poison_remaining}
			};
		}

		[[nodiscard]] nlohmann::json combatEventToJson(const CombatEvent& event) {
			return {
				{"sequence_id", event.sequence_id},
				{"time_seconds", event.time_seconds},
				{"event_type", toString(event.type)},
				{"source", entityRefToJson(event.source)},
				{"target", entityRefToJson(event.target)},
				{"source_label", event.source_label},
				{"amount", event.amount},
				{"hp_before", event.hp_before},
				{"hp_after", event.hp_after},
				{"lethal", event.lethal},
				{"has_target_position", event.has_target_position},
				{"target_position", vectorToJson(event.target_position)},
				{"event_position", vectorToJson(event.event_position)}
			};
		}

#ifdef _WIN32
		[[nodiscard]] SOCKET toNativeSocket(const TelemetrySocketHandle socket) {
			return static_cast<SOCKET>(socket);
		}

		[[nodiscard]] TelemetrySocketHandle toSocketHandle(const SOCKET socket) {
			return static_cast<TelemetrySocketHandle>(socket);
		}

		[[nodiscard]] std::string socketErrorMessage(const char* context) {
			return std::string(context) + " failed with WSA error " + std::to_string(WSAGetLastError());
		}
#endif
	}

	CombatTelemetryServer::~CombatTelemetryServer() {
		stop();
	}

	bool CombatTelemetryServer::start() {
		return start(Settings{});
	}

	bool CombatTelemetryServer::start(const Settings& settings) {
		if (_running.load())
			return true;

		_settings = settings;
		_settings.max_queued_messages = std::max<size_t>(1, _settings.max_queued_messages);

#ifdef _WIN32
		WSADATA wsa_data{};
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
			setLastError(socketErrorMessage("WSAStartup"));
			return false;
		}

		if (!openListenSocket()) {
			WSACleanup();
			return false;
		}

		_running.store(true);
		_worker = std::thread(&CombatTelemetryServer::workerLoop, this);
		return true;
#else
		setLastError("CombatTelemetryServer currently supports Windows builds only.");
		return false;
#endif
	}

	void CombatTelemetryServer::stop() {
		if (!_running.exchange(false))
			return;

		if (_worker.joinable())
			_worker.join();

#ifdef _WIN32
		closeClientSockets();
		closeListenSocket();
		WSACleanup();
#endif

		std::lock_guard lock(_mutex);
		_queued_messages.clear();
	}

	void CombatTelemetryServer::publish(const CombatEvent& event) {
		if (!_running.load())
			return;

		queueMessage(serializeEvent(event));
	}

	void CombatTelemetryServer::publishAgentPerception(const AgentPerceptionSnapshot& snapshot) {
		if (!_running.load())
			return;

		queueMessage(serializeAgentPerception(snapshot));
	}

	void CombatTelemetryServer::queueMessage(std::string payload) {
		std::lock_guard lock(_mutex);
		if (_queued_messages.size() >= _settings.max_queued_messages)
			_queued_messages.pop_front();
		_queued_messages.push_back(std::move(payload));
	}

	std::string CombatTelemetryServer::getLastError() const {
		std::lock_guard lock(_mutex);
		return _last_error;
	}

	std::string CombatTelemetryServer::serializeEvent(const CombatEvent& event) const {
		nlohmann::json json_event = combatEventToJson(event);
		json_event["schema"] = "nawia.telemetry.combat.v1";

		return json_event.dump() + '\n';
	}

	std::string CombatTelemetryServer::serializeAgentPerception(const AgentPerceptionSnapshot& snapshot) const {
		nlohmann::json observed_entities = nlohmann::json::array();
		for (const auto& observed : snapshot.observed_entities) {
			observed_entities.push_back({
				{"entity", agentEntityToJson(observed.entity)},
				{"relation", toString(observed.relation)},
				{"distance", observed.distance},
				{"direction", vectorToJson(observed.direction)},
				{"is_current_target", observed.is_current_target}
			});
		}

		nlohmann::json abilities = nlohmann::json::array();
		for (const auto& ability : snapshot.abilities) {
			abilities.push_back({
				{"slot", ability.slot},
				{"name", ability.name},
				{"target_type", toString(ability.target_type)},
				{"ready", ability.ready},
				{"can_cast", ability.can_cast},
				{"cooldown_remaining", ability.cooldown_remaining},
				{"cooldown_ratio", ability.cooldown_ratio},
				{"cooldown", ability.cooldown},
				{"cast_range", ability.cast_range},
				{"duration", ability.duration},
				{"projectile_speed", ability.projectile_speed},
				{"hitbox_radius", ability.hitbox_radius},
				{"damage", ability.damage}
			});
		}

		nlohmann::json lost_entities = nlohmann::json::array();
		for (const auto& lost : snapshot.lost_entities) {
			lost_entities.push_back({
				{"last_known_entity", agentEntityToJson(lost.last_known_entity)},
				{"relation", toString(lost.relation)},
				{"last_known_position", vectorToJson(lost.last_known_position)},
				{"last_seen_time_seconds", lost.last_seen_time_seconds},
				{"seconds_since_seen", lost.seconds_since_seen},
				{"was_current_target", lost.was_current_target},
				{"disappearance_reason", lost.disappearance_reason}
			});
		}

		nlohmann::json recent_events = nlohmann::json::array();
		for (const auto& event : snapshot.recent_combat_events)
			recent_events.push_back(combatEventToJson(event));

		nlohmann::json json_snapshot = {
			{"schema", "nawia.telemetry.agent_perception.v1"},
			{"frame_id", snapshot.frame_id},
			{"time_seconds", snapshot.time_seconds},
			{"perception_radius", snapshot.perception_radius},
			{"event_memory_seconds", snapshot.event_memory_seconds},
			{"agent", agentEntityToJson(snapshot.self)},
			{"current_target", snapshot.current_target ? agentEntityToJson(*snapshot.current_target) : nlohmann::json(nullptr)},
			{"last_damage_source", snapshot.last_damage_source ? agentEntityToJson(*snapshot.last_damage_source) : nlohmann::json(nullptr)},
			{"observed_entities", observed_entities},
			{"lost_entities", lost_entities},
			{"abilities", abilities},
			{"recent_combat_events", recent_events},
			{"nearby_enemy_count", snapshot.nearby_enemy_count},
			{"nearby_ally_count", snapshot.nearby_ally_count},
			{"nearby_neutral_count", snapshot.nearby_neutral_count},
			{"nearby_npc_count", snapshot.nearby_npc_count},
			{"nearby_projectile_count", snapshot.nearby_projectile_count},
			{"lost_entity_count", snapshot.lost_entity_count}
		};

		return json_snapshot.dump() + '\n';
	}

	void CombatTelemetryServer::setLastError(std::string error) {
		std::lock_guard lock(_mutex);
		_last_error = std::move(error);
	}

#ifdef _WIN32
	bool CombatTelemetryServer::openListenSocket() {
		const SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listen_socket == INVALID_SOCKET) {
			setLastError(socketErrorMessage("socket"));
			return false;
		}
		_listen_socket = toSocketHandle(listen_socket);

		const BOOL reuse_address = TRUE;
		setsockopt(toNativeSocket(_listen_socket), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse_address), sizeof(reuse_address));

		u_long non_blocking = 1;
		if (ioctlsocket(toNativeSocket(_listen_socket), FIONBIO, &non_blocking) == SOCKET_ERROR) {
			setLastError(socketErrorMessage("ioctlsocket"));
			closeListenSocket();
			return false;
		}

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(_settings.port);
		if (inet_pton(AF_INET, _settings.host.c_str(), &address.sin_addr) != 1) {
			setLastError("Invalid telemetry host: " + _settings.host);
			closeListenSocket();
			return false;
		}

		if (bind(toNativeSocket(_listen_socket), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
			setLastError(socketErrorMessage("bind"));
			closeListenSocket();
			return false;
		}

		if (listen(toNativeSocket(_listen_socket), SOMAXCONN) == SOCKET_ERROR) {
			setLastError(socketErrorMessage("listen"));
			closeListenSocket();
			return false;
		}

		return true;
	}

	void CombatTelemetryServer::workerLoop() {
		while (_running.load()) {
			acceptPendingClients();
			flushQueuedMessages();
			std::this_thread::sleep_for(WORKER_SLEEP);
		}
	}

	void CombatTelemetryServer::acceptPendingClients() {
		if (_listen_socket == INVALID_TELEMETRY_SOCKET)
			return;

		while (_running.load()) {
			const SOCKET client_socket = accept(toNativeSocket(_listen_socket), nullptr, nullptr);
			if (client_socket == INVALID_SOCKET) {
				const int error = WSAGetLastError();
				if (error != WSAEWOULDBLOCK)
					setLastError(socketErrorMessage("accept"));
				return;
			}

			u_long non_blocking = 1;
			if (ioctlsocket(client_socket, FIONBIO, &non_blocking) == SOCKET_ERROR) {
				closesocket(client_socket);
				continue;
			}

			_client_sockets.push_back(toSocketHandle(client_socket));
		}
	}

	void CombatTelemetryServer::flushQueuedMessages() {
		if (_client_sockets.empty())
			return;

		std::vector<std::string> messages;
		{
			std::lock_guard lock(_mutex);
			messages.assign(_queued_messages.begin(), _queued_messages.end());
			_queued_messages.clear();
		}

		for (const auto& message : messages) {
			for (size_t index = 0; index < _client_sockets.size();) {
				if (sendMessage(_client_sockets[index], message))
					++index;
				else
					dropClientAt(index);
			}
		}
	}

	void CombatTelemetryServer::closeListenSocket() {
		if (_listen_socket == INVALID_TELEMETRY_SOCKET)
			return;

		closesocket(toNativeSocket(_listen_socket));
		_listen_socket = INVALID_TELEMETRY_SOCKET;
	}

	void CombatTelemetryServer::closeClientSockets() {
		for (const TelemetrySocketHandle client_socket : _client_sockets)
			closesocket(toNativeSocket(client_socket));
		_client_sockets.clear();
	}

	void CombatTelemetryServer::dropClientAt(const size_t index) {
		if (index >= _client_sockets.size())
			return;

		closesocket(toNativeSocket(_client_sockets[index]));
		_client_sockets.erase(_client_sockets.begin() + static_cast<std::ptrdiff_t>(index));
	}

	bool CombatTelemetryServer::sendMessage(const TelemetrySocketHandle client_socket, const std::string& message) {
		size_t sent_total = 0;
		while (sent_total < message.size()) {
			const int chunk_size = static_cast<int>(std::min<size_t>(
				message.size() - sent_total,
				static_cast<size_t>(std::numeric_limits<int>::max())
			));
			const int sent = send(toNativeSocket(client_socket), message.data() + sent_total, chunk_size, 0);

			if (sent > 0) {
				sent_total += static_cast<size_t>(sent);
				continue;
			}

			if (sent == 0)
				return false;

			const int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK)
				return true;

			return false;
		}

		return true;
	}
#endif

} // namespace Nawia::Game
