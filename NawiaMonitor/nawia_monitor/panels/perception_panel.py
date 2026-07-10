from time import monotonic
from typing import Any, Callable

from PyQt6.QtWidgets import (
	QComboBox,
	QFrame,
	QHBoxLayout,
	QLabel,
	QSplitter,
	QTabWidget,
	QTableWidget,
	QTableWidgetItem,
	QVBoxLayout,
	QWidget,
)
from PyQt6.QtCore import Qt, QTimer

from .. import telemetry_format as fmt
from ..table_utils import make_readonly_table, set_table_rows


PERCEPTION_STALE_SECONDS = 1.5


class PerceptionPanel(QFrame):
	def __init__(self, show_details: Callable[[dict[str, Any]], None], parent: QWidget | None = None) -> None:
		super().__init__(parent)
		self.setObjectName("panel")
		self._show_details = show_details
		self._rows: dict[int, int] = {}
		self._snapshots: dict[int, dict[str, Any]] = {}
		self._last_seen: dict[int, float] = {}
		self._selected_agent_id = 0
		self._focused_agent_id = 0

		self._build_ui()

		self._cleanup_timer = QTimer(self)
		self._cleanup_timer.setInterval(500)
		self._cleanup_timer.timeout.connect(self.remove_stale_rows)
		self._cleanup_timer.start()

	def _build_ui(self) -> None:
		layout = QVBoxLayout(self)

		header = QHBoxLayout()
		title = QLabel("Agent Perception")
		title.setObjectName("panelTitle")
		self._agent_combo = QComboBox()
		self._agent_combo.setMinimumWidth(260)
		self._agent_combo.currentIndexChanged.connect(self._on_agent_filter_changed)
		header.addWidget(title)
		header.addStretch(1)
		header.addWidget(QLabel("Agent"))
		header.addWidget(self._agent_combo)
		layout.addLayout(header)

		self._focus_label = QLabel("No agent selected")
		self._focus_label.setObjectName("hintLabel")
		layout.addWidget(self._focus_label)

		self._table = make_readonly_table([
			"Frame",
			"Time",
			"Agent",
			"HP",
			"Visible",
			"Interact",
			"Target",
			"Seen",
			"Lost",
			"Info Pings",
			"Threat Pings",
			"Ping Memory",
			"Enemies",
			"Allies",
			"NPCs",
			"Hazards",
			"Events",
			"Ready Abilities",
		])
		self._table.itemSelectionChanged.connect(self._show_selected_perception)

		splitter = QSplitter(Qt.Orientation.Vertical, self)
		splitter.addWidget(self._table)
		splitter.addWidget(self._build_detail_tabs())
		splitter.setSizes([220, 420])
		layout.addWidget(splitter, 1)

	def _build_detail_tabs(self) -> QWidget:
		tabs = QTabWidget()

		self._observed_entities_table = make_readonly_table([
			"Name",
			"Type",
			"Faction",
			"Relation",
			"HP",
			"Distance",
			"Position",
			"Flags",
			"Interact",
			"Target",
		])
		tabs.addTab(self._observed_entities_table, "Seen Entities")

		self._lost_entities_table = make_readonly_table([
			"Name",
			"Type",
			"Faction",
			"Relation",
			"Last Position",
			"Ago",
			"Reason",
			"Was Target",
			"Interact",
		])
		tabs.addTab(self._lost_entities_table, "Lost Memory")

		combat_panel = QWidget()
		combat_layout = QVBoxLayout(combat_panel)
		combat_layout.setContentsMargins(0, 0, 0, 0)
		self._incoming_damage_label = QLabel("Incoming damage: -")
		self._incoming_damage_label.setObjectName("hintLabel")
		combat_layout.addWidget(self._incoming_damage_label)
		self._combat_table = make_readonly_table([
			"Time",
			"Direction",
			"Type",
			"Source",
			"Target",
			"Amount",
			"HP",
			"Label",
		])
		combat_layout.addWidget(self._combat_table, 1)
		tabs.addTab(combat_panel, "Combat")

		self._pings_table = make_readonly_table([
			"Memory",
			"Type",
			"Source",
			"Age",
			"TTL",
			"Position",
			"Active",
		])
		tabs.addTab(self._pings_table, "Pings")

		self._abilities_table = make_readonly_table([
			"Slot",
			"Name",
			"Target",
			"Ready",
			"Can Cast",
			"Cooldown",
			"Range",
			"Damage",
		])
		tabs.addTab(self._abilities_table, "Abilities")

		return tabs

	def clear(self) -> None:
		self._rows.clear()
		self._snapshots.clear()
		self._last_seen.clear()
		self._selected_agent_id = 0
		self._focused_agent_id = 0
		self._refresh_agent_combo()
		self._refresh_table()
		self._refresh_details()

	def upsert(self, snapshot: dict[str, Any]) -> None:
		agent = snapshot.get("agent")
		if not isinstance(agent, dict):
			return

		agent_id = fmt.entity_id(agent)
		if agent_id == 0:
			return

		self._snapshots[agent_id] = snapshot
		self._last_seen[agent_id] = monotonic()
		self._refresh_agent_combo()
		self._refresh_table()
		self._refresh_details()

	def remove_stale_rows(self) -> None:
		now = monotonic()
		stale_ids = [
			entity_id
			for entity_id, last_seen in self._last_seen.items()
			if now - last_seen > PERCEPTION_STALE_SECONDS
		]
		if not stale_ids:
			return

		for entity_id in stale_ids:
			self._rows.pop(entity_id, None)
			self._snapshots.pop(entity_id, None)
			self._last_seen.pop(entity_id, None)

		if self._selected_agent_id in stale_ids:
			self._selected_agent_id = 0
		if self._focused_agent_id in stale_ids:
			self._focused_agent_id = 0
		self._refresh_agent_combo()
		self._refresh_table()
		self._refresh_details()

	def _refresh_agent_combo(self) -> None:
		current_id = self._selected_agent_id
		self._agent_combo.blockSignals(True)
		self._agent_combo.clear()
		self._agent_combo.addItem("All agents", 0)
		for entity_id, snapshot in self._sorted_snapshots():
			self._agent_combo.addItem(fmt.entity_label(snapshot.get("agent")), entity_id)

		index = self._agent_combo.findData(current_id)
		if index < 0:
			self._selected_agent_id = 0
			index = 0
		self._agent_combo.setCurrentIndex(index)
		self._agent_combo.blockSignals(False)

	def _on_agent_filter_changed(self, index: int) -> None:
		if index < 0:
			return

		value = self._agent_combo.itemData(index)
		self._selected_agent_id = int(value or 0)
		if self._selected_agent_id:
			self._focused_agent_id = self._selected_agent_id
		self._refresh_table()
		self._refresh_details()

	def _refresh_table(self) -> None:
		visible_snapshots = [
			(entity_id, snapshot)
			for entity_id, snapshot in self._sorted_snapshots()
			if not self._selected_agent_id or entity_id == self._selected_agent_id
		]
		visible_ids = {entity_id for entity_id, _ in visible_snapshots}
		if self._selected_agent_id in visible_ids:
			self._focused_agent_id = self._selected_agent_id
		elif self._focused_agent_id not in visible_ids:
			self._focused_agent_id = visible_snapshots[0][0] if visible_snapshots else 0

		self._rows.clear()
		self._table.blockSignals(True)
		self._table.setRowCount(0)
		focused_row = -1
		for entity_id, snapshot in visible_snapshots:
			row = self._table.rowCount()
			self._table.insertRow(row)
			self._rows[entity_id] = row
			self._populate_row(row, snapshot)
			if self._focused_agent_id == entity_id:
				focused_row = row
		self._table.blockSignals(False)

		if focused_row >= 0:
			self._table.selectRow(focused_row)
		else:
			self._table.clearSelection()

	def _sorted_snapshots(self) -> list[tuple[int, dict[str, Any]]]:
		return sorted(
			self._snapshots.items(),
			key=lambda item: fmt.entity_label(item[1].get("agent")),
		)

	def _populate_row(self, row: int, snapshot: dict[str, Any]) -> None:
		agent = snapshot.get("agent")
		if not isinstance(agent, dict):
			return

		ready_abilities = [
			str(ability.get("name"))
			for ability in snapshot.get("abilities", [])
			if isinstance(ability, dict) and ability.get("ready")
		]
		current_target = snapshot.get("current_target")
		visible_pings = [
			ping for ping in snapshot.get("visible_pings", [])
			if isinstance(ping, dict)
		]
		info_ping_count = sum(1 for ping in visible_pings if ping.get("ping_type") == "Info")
		threat_ping_count = sum(1 for ping in visible_pings if ping.get("ping_type") == "Threat")
		values = [
			snapshot.get("frame_id", ""),
			f"{float(snapshot.get('time_seconds', 0.0)):.2f}",
			fmt.entity_label(agent),
			f"{agent.get('hp', '')}/{agent.get('max_hp', '')}",
			"yes" if agent.get("visible") else "no",
			fmt.interaction_text(agent),
			fmt.entity_label(current_target),
			len(snapshot.get("observed_entities", [])),
			len(snapshot.get("lost_entities", [])),
			info_ping_count,
			threat_ping_count,
			len(snapshot.get("remembered_pings", [])),
			snapshot.get("nearby_enemy_count", 0),
			snapshot.get("nearby_ally_count", 0),
			snapshot.get("nearby_npc_count", 0),
			snapshot.get("nearby_hazard_count", 0),
			len(snapshot.get("recent_combat_events", [])),
			", ".join(ready_abilities) if ready_abilities else "-",
		]

		for column, value in enumerate(values):
			item = self._table.item(row, column)
			if item is None:
				item = QTableWidgetItem()
				self._table.setItem(row, column, item)
			item.setText(str(value))
			if column in (0, 1, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16):
				item.setTextAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)

	def _refresh_details(self) -> None:
		snapshot = self._snapshots.get(self._focused_agent_id)
		if not snapshot:
			self._focus_label.setText("No agent selected")
			self._incoming_damage_label.setText("Incoming damage: -")
			set_table_rows(self._observed_entities_table, [])
			set_table_rows(self._lost_entities_table, [])
			set_table_rows(self._combat_table, [])
			set_table_rows(self._pings_table, [])
			set_table_rows(self._abilities_table, [])
			return

		agent = snapshot.get("agent")
		agent_id = fmt.entity_id(agent)
		current_target = snapshot.get("current_target")
		last_damage_source = snapshot.get("last_damage_source")
		self._focus_label.setText(
			"Focus: "
			f"{fmt.entity_label(agent)} | "
			f"Target: {fmt.entity_label(current_target)} | "
			f"Last damage source: {fmt.entity_label(last_damage_source)} | "
			f"Lost memory: {fmt.float_text(snapshot.get('lost_memory_seconds'))}s"
		)

		self._refresh_observed(snapshot)
		self._refresh_lost(snapshot)
		self._refresh_combat(snapshot, agent_id)
		self._refresh_pings(snapshot)
		self._refresh_abilities(snapshot)

	def _refresh_observed(self, snapshot: dict[str, Any]) -> None:
		rows: list[list[Any]] = []
		for observed in snapshot.get("observed_entities", []):
			if not isinstance(observed, dict):
				continue

			entity = observed.get("entity")
			rows.append([
				fmt.entity_name(entity),
				fmt.entity_field(entity, "entity_type"),
				fmt.entity_field(entity, "faction"),
				observed.get("relation", "-"),
				fmt.entity_hp(entity),
				fmt.float_text(observed.get("distance")),
				fmt.position_text(entity.get("position") if isinstance(entity, dict) else None),
				fmt.entity_flags(entity),
				fmt.interaction_text(entity),
				"yes" if observed.get("is_current_target") else "",
			])
		set_table_rows(self._observed_entities_table, rows, {5})

	def _refresh_lost(self, snapshot: dict[str, Any]) -> None:
		rows: list[list[Any]] = []
		for lost in snapshot.get("lost_entities", []):
			if not isinstance(lost, dict):
				continue

			entity = lost.get("last_known_entity")
			rows.append([
				fmt.entity_name(entity),
				fmt.entity_field(entity, "entity_type"),
				fmt.entity_field(entity, "faction"),
				lost.get("relation", "-"),
				fmt.position_text(lost.get("last_known_position")),
				f"{fmt.float_text(lost.get('seconds_since_seen'))}s",
				lost.get("disappearance_reason", "-"),
				"yes" if lost.get("was_current_target") else "",
				fmt.interaction_text(entity),
			])
		set_table_rows(self._lost_entities_table, rows, {5})

	def _refresh_combat(self, snapshot: dict[str, Any], agent_id: int) -> None:
		combat_events = [
			event for event in snapshot.get("recent_combat_events", [])
			if isinstance(event, dict)
		]
		incoming_events = [
			event for event in combat_events
			if event.get("event_type") == "DamageDealt" and fmt.entity_id(event.get("target")) == agent_id
		]
		if incoming_events:
			last_incoming = incoming_events[-1]
			self._incoming_damage_label.setText(
				"Incoming damage: "
				f"{fmt.entity_label(last_incoming.get('source'))} hit for {last_incoming.get('amount', '-')} "
				f"({last_incoming.get('source_label') or '-'}) "
				f"HP {fmt.hp_text(last_incoming)}"
			)
		else:
			self._incoming_damage_label.setText("Incoming damage: none in recent perception window")

		rows: list[list[Any]] = []
		for event in reversed(combat_events):
			rows.append([
				f"{float(event.get('time_seconds', 0.0)):.2f}",
				fmt.event_direction(event, agent_id),
				event.get("event_type", "-"),
				fmt.entity_label(event.get("source")),
				fmt.entity_label(event.get("target")),
				event.get("amount", ""),
				fmt.hp_text(event),
				event.get("source_label", ""),
			])
		set_table_rows(self._combat_table, rows, {0, 5})

	def _refresh_pings(self, snapshot: dict[str, Any]) -> None:
		rows: list[list[Any]] = []
		for memory_label, key in (("Visible", "visible_pings"), ("Remembered", "remembered_pings")):
			for ping in snapshot.get(key, []):
				if not isinstance(ping, dict):
					continue

				age = float(ping.get("age_seconds", 0.0))
				duration = float(ping.get("duration_seconds", 0.0))
				ttl = max(0.0, duration - age)
				rows.append([
					memory_label,
					ping.get("ping_type", "-"),
					fmt.entity_label(ping.get("source")),
					f"{age:.2f}s",
					f"{ttl:.2f}s",
					fmt.position_text(ping.get("position")),
					"yes" if ping.get("active") else "no",
				])
		set_table_rows(self._pings_table, rows, {3, 4})

	def _refresh_abilities(self, snapshot: dict[str, Any]) -> None:
		rows: list[list[Any]] = []
		for ability in snapshot.get("abilities", []):
			if not isinstance(ability, dict):
				continue

			rows.append([
				ability.get("slot", ""),
				ability.get("name", "-"),
				ability.get("target_type", "-"),
				"yes" if ability.get("ready") else "no",
				"yes" if ability.get("can_cast") else "no",
				fmt.cooldown_text(ability),
				fmt.float_text(ability.get("cast_range")),
				ability.get("damage", ""),
			])
		set_table_rows(self._abilities_table, rows, {0, 5, 6, 7})

	def _show_selected_perception(self) -> None:
		rows = self._table.selectionModel().selectedRows()
		if not rows:
			return

		row = rows[0].row()
		for entity_id, stored_row in self._rows.items():
			if stored_row == row:
				snapshot = self._snapshots.get(entity_id)
				if snapshot:
					self._focused_agent_id = entity_id
					self._refresh_details()
					self._show_details(snapshot)
				return
