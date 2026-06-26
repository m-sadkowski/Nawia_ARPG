#pragma once

#include <AgentPerceptionSystem.h>
#include <CombatEventBus.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace Nawia::Game {

#ifdef _WIN32
	using TelemetrySocketHandle = std::uintptr_t;
	inline constexpr TelemetrySocketHandle INVALID_TELEMETRY_SOCKET = ~TelemetrySocketHandle{0};
#endif

	/**
	 * @class CombatTelemetryServer
	 * @brief Best-effort localhost telemetry bridge for external ET tools.
	 *
	 * The server serializes CombatEventBus events as newline-delimited JSON
	 * and sends them to any local TCP client connected to 127.0.0.1:19777.
	 * It deliberately does not own gameplay logic; dropped telemetry packets
	 * must never affect the simulation.
	 */
	class CombatTelemetryServer {
	public:
		struct Settings {
			std::string host = "127.0.0.1";
			std::uint16_t port = 19777;
			size_t max_queued_messages = 512;
		};

		CombatTelemetryServer() = default;
		~CombatTelemetryServer();

		CombatTelemetryServer(const CombatTelemetryServer&) = delete;
		CombatTelemetryServer& operator=(const CombatTelemetryServer&) = delete;

		bool start();
		bool start(const Settings& settings);
		void stop();

		void publish(const CombatEvent& event);
		void publishAgentPerception(const AgentPerceptionSnapshot& snapshot);

		[[nodiscard]] bool isRunning() const { return _running.load(); }
		[[nodiscard]] std::uint16_t getPort() const { return _settings.port; }
		[[nodiscard]] std::string getLastError() const;

	private:
		[[nodiscard]] std::string serializeEvent(const CombatEvent& event) const;
		[[nodiscard]] std::string serializeAgentPerception(const AgentPerceptionSnapshot& snapshot) const;
		void setLastError(std::string error);
		void queueMessage(std::string payload);

#ifdef _WIN32
		bool openListenSocket();
		void workerLoop();
		void acceptPendingClients();
		void flushQueuedMessages();
		void closeListenSocket();
		void closeClientSockets();
		void dropClientAt(size_t index);
		[[nodiscard]] bool sendMessage(TelemetrySocketHandle client_socket, const std::string& message);

		TelemetrySocketHandle _listen_socket = INVALID_TELEMETRY_SOCKET;
		std::deque<TelemetrySocketHandle> _client_sockets;
#endif

		Settings _settings;
		std::atomic_bool _running = false;
		std::thread _worker;
		mutable std::mutex _mutex;
		std::deque<std::string> _queued_messages;
		std::string _last_error;
	};

} // namespace Nawia::Game
