#include "AgentCommandInterface.h"

namespace Nawia::Game {

	const char* toString(const AgentCommandType type) {
		switch (type) {
			case AgentCommandType::None:
				return "None";
			case AgentCommandType::MoveTo:
				return "MoveTo";
			case AgentCommandType::MoveToEntity:
				return "MoveToEntity";
			case AgentCommandType::Stop:
				return "Stop";
			case AgentCommandType::Attack:
				return "Attack";
			case AgentCommandType::CastAbility:
				return "CastAbility";
			case AgentCommandType::Interact:
				return "Interact";
		}

		return "Unknown";
	}

	const char* toString(const AgentCommandStatus status) {
		switch (status) {
			case AgentCommandStatus::Queued:
				return "Queued";
			case AgentCommandStatus::Running:
				return "Running";
			case AgentCommandStatus::Succeeded:
				return "Succeeded";
			case AgentCommandStatus::Failed:
				return "Failed";
			case AgentCommandStatus::Cancelled:
				return "Cancelled";
		}

		return "Unknown";
	}

	const char* toString(const AgentCommandFailureReason reason) {
		switch (reason) {
			case AgentCommandFailureReason::None:
				return "None";
			case AgentCommandFailureReason::NoAgent:
				return "NoAgent";
			case AgentCommandFailureReason::AgentUnavailable:
				return "AgentUnavailable";
			case AgentCommandFailureReason::NoTarget:
				return "NoTarget";
			case AgentCommandFailureReason::TargetUnavailable:
				return "TargetUnavailable";
			case AgentCommandFailureReason::TargetOutOfRange:
				return "TargetOutOfRange";
			case AgentCommandFailureReason::NoAbility:
				return "NoAbility";
			case AgentCommandFailureReason::AbilityOnCooldown:
				return "AbilityOnCooldown";
			case AgentCommandFailureReason::AbilityTargetMismatch:
				return "AbilityTargetMismatch";
			case AgentCommandFailureReason::ControlLocked:
				return "ControlLocked";
			case AgentCommandFailureReason::MovementRooted:
				return "MovementRooted";
			case AgentCommandFailureReason::InteractionUnavailable:
				return "InteractionUnavailable";
			case AgentCommandFailureReason::PathUnavailable:
				return "PathUnavailable";
			case AgentCommandFailureReason::CommandReplaced:
				return "CommandReplaced";
			case AgentCommandFailureReason::Cancelled:
				return "Cancelled";
		}

		return "Unknown";
	}

} // namespace Nawia::Game
