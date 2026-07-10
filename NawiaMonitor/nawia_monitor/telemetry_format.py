from typing import Any


def entity_id(value: Any) -> int:
	if not isinstance(value, dict):
		return 0
	return int(value.get("entity_id") or value.get("runtime_id") or 0)


def entity_field(value: Any, field: str) -> str:
	if not isinstance(value, dict) or not value.get("valid"):
		return "-"
	return str(value.get(field) or "-")


def entity_name(value: Any) -> str:
	if not isinstance(value, dict) or not value.get("valid"):
		return "-"

	name = str(value.get("name") or value.get("entity_type") or "-")
	value_id = entity_id(value)
	return f"{name}#{value_id}" if value_id else name


def entity_label(value: Any) -> str:
	if not isinstance(value, dict) or not value.get("valid"):
		return "-"

	name = entity_name(value)
	entity_type = str(value.get("entity_type") or "?")
	faction = str(value.get("faction") or "?")
	return f"{name} ({entity_type}/{faction})"


def entity_hp(value: Any) -> str:
	if not isinstance(value, dict) or not value.get("valid"):
		return "-"
	return f"{value.get('hp', '')}/{value.get('max_hp', '')}"


def entity_flags(value: Any) -> str:
	if not isinstance(value, dict) or not value.get("valid"):
		return "-"

	flags: list[str] = []
	if not value.get("alive", True):
		flags.append("dead")
	if value.get("dying"):
		flags.append("dying")
	if value.get("dormant"):
		flags.append("dormant")
	if not value.get("visible", True):
		flags.append("hidden")
	if value.get("moving"):
		flags.append("moving")
	if value.get("rooted"):
		flags.append(f"rooted {float_text(value.get('root_remaining'))}s")
	if value.get("poisoned"):
		flags.append(f"poisoned {float_text(value.get('poison_remaining'))}s")
	if value.get("casting"):
		cast_name = str(value.get("cast_name") or "cast")
		flags.append(f"casting {cast_name} {float_text(value.get('cast_remaining'))}s")
	if value.get("hazard"):
		phase = str(value.get("hazard_phase") or "Hazard").lower()
		radius = float_text(value.get("hazard_radius"))
		if value.get("hazard_expanding_wave"):
			current_radius = float_text(value.get("hazard_current_radius"))
			radius = f"{current_radius}/{radius}"
		damage = value.get("hazard_damage_per_tick", 0)
		effect = " knockdown" if value.get("hazard_knock_down_player_on_hit") else ""
		if phase == "warning":
			kind = "wave warning" if value.get("hazard_expanding_wave") else "hazard warning"
			flags.append(f"{kind} {float_text(value.get('hazard_time_to_activate'))}s r{radius}{effect}")
		else:
			kind = f"wave {phase}" if value.get("hazard_expanding_wave") else f"hazard {phase}"
			flags.append(f"{kind} {float_text(value.get('hazard_remaining'))}s r{radius} dmg {damage}{effect}")
	return ", ".join(flags) if flags else "-"


def interaction_text(value: Any) -> str:
	if not isinstance(value, dict) or not value.get("interactable"):
		return "-"

	state = str(value.get("interaction_state") or "Interactable")
	available = "yes" if value.get("interaction_available") else "no"
	return f"{state}/{available}"


def event_direction(event: dict[str, Any], agent_id: int) -> str:
	source_id = entity_id(event.get("source"))
	target_id = entity_id(event.get("target"))
	if target_id == agent_id and source_id == agent_id:
		return "self"
	if target_id == agent_id:
		return "incoming"
	if source_id == agent_id:
		return "outgoing"
	return "nearby"


def position_text(value: Any) -> str:
	if not isinstance(value, dict):
		return "-"

	x = float_text(value.get("x"))
	y = float_text(value.get("y"))
	z = value.get("z")
	if z is None:
		return f"{x}, {y}"
	return f"{x}, {y}, {float_text(z)}"


def float_text(value: Any) -> str:
	try:
		return f"{float(value):.2f}"
	except (TypeError, ValueError):
		return "-"


def cooldown_text(ability: dict[str, Any]) -> str:
	remaining = float_text(ability.get("cooldown_remaining"))
	cooldown = float_text(ability.get("cooldown"))
	return f"{remaining}/{cooldown}"


def hp_text(event: dict[str, Any]) -> str:
	if event.get("event_type") != "DamageDealt":
		return ""
	return f"{event.get('hp_before', '')}->{event.get('hp_after', '')}"
