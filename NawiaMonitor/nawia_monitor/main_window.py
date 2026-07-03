import json
from collections import Counter
from time import monotonic
from typing import Any

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import (
	QAbstractItemView,
	QApplication,
	QComboBox,
	QFormLayout,
	QFrame,
	QGridLayout,
	QHeaderView,
	QHBoxLayout,
	QLabel,
	QLineEdit,
	QMainWindow,
	QPlainTextEdit,
	QPushButton,
	QSpinBox,
	QSplitter,
	QTabWidget,
	QTableWidget,
	QTableWidgetItem,
	QVBoxLayout,
	QWidget,
)

from .telemetry_client import TelemetryClient


MAX_EVENTS = 1000
PERCEPTION_STALE_SECONDS = 1.5


class MainWindow(QMainWindow):
	def __init__(self) -> None:
		super().__init__()
		self.setWindowTitle("Nawia ET Monitor")
		self.resize(1280, 720)
		self.setMinimumSize(960, 540)

		self._events: list[dict[str, Any]] = []
		self._event_counts: Counter[str] = Counter()
		self._perception_rows: dict[int, int] = {}
		self._perception_snapshots: dict[int, dict[str, Any]] = {}
		self._perception_last_seen: dict[int, float] = {}
		self._selected_perception_agent_id = 0
		self._focused_perception_agent_id = 0
		self._client = TelemetryClient(parent=self)
		self._client.event_received.connect(self._handle_telemetry_message)
		self._client.connection_changed.connect(self._set_connected)
		self._client.status_changed.connect(self._status_label_text)
		self._client.error_received.connect(self._error_label_text)

		self._build_ui()
		self._apply_style()
		self._perception_cleanup_timer = QTimer(self)
		self._perception_cleanup_timer.setInterval(500)
		self._perception_cleanup_timer.timeout.connect(self._remove_stale_perception_rows)
		self._perception_cleanup_timer.start()
		self._client.start()

	def _build_ui(self) -> None:
		root = QWidget(self)
		layout = QVBoxLayout(root)
		layout.setContentsMargins(10, 10, 10, 10)
		layout.setSpacing(8)

		layout.addLayout(self._build_toolbar())

		splitter = QSplitter(Qt.Orientation.Horizontal, root)
		splitter.addWidget(self._build_metrics_panel())
		splitter.addWidget(self._build_center_tabs())
		splitter.addWidget(self._build_details_panel())
		splitter.setSizes([220, 720, 340])
		layout.addWidget(splitter, 1)

		self.setCentralWidget(root)

	def _build_toolbar(self) -> QHBoxLayout:
		toolbar = QHBoxLayout()
		toolbar.setSpacing(8)

		self._status_label = QLabel("Disconnected")
		self._status_label.setObjectName("statusLabel")

		self._error_label = QLabel("")
		self._error_label.setObjectName("errorLabel")

		self._host_input = QLineEdit("127.0.0.1")
		self._host_input.setFixedWidth(140)

		self._port_input = QSpinBox()
		self._port_input.setRange(1, 65535)
		self._port_input.setValue(19777)
		self._port_input.setFixedWidth(90)

		self._connect_button = QPushButton("Reconnect")
		self._connect_button.clicked.connect(self._reconnect)

		self._clear_button = QPushButton("Clear")
		self._clear_button.clicked.connect(self._clear_events)

		toolbar.addWidget(QLabel("Host"))
		toolbar.addWidget(self._host_input)
		toolbar.addWidget(QLabel("Port"))
		toolbar.addWidget(self._port_input)
		toolbar.addWidget(self._connect_button)
		toolbar.addWidget(self._clear_button)
		toolbar.addSpacing(12)
		toolbar.addWidget(self._status_label)
		toolbar.addWidget(self._error_label, 1)
		return toolbar

	def _build_metrics_panel(self) -> QWidget:
		panel = QFrame()
		panel.setObjectName("panel")
		layout = QVBoxLayout(panel)

		title = QLabel("Telemetry")
		title.setObjectName("panelTitle")
		layout.addWidget(title)

		form = QFormLayout()
		form.setLabelAlignment(Qt.AlignmentFlag.AlignLeft)
		self._total_events_label = QLabel("0")
		self._damage_events_label = QLabel("0")
		self._kill_events_label = QLabel("0")
		self._cast_events_label = QLabel("0")
		self._last_event_label = QLabel("-")
		form.addRow("Events", self._total_events_label)
		form.addRow("Damage", self._damage_events_label)
		form.addRow("Kills", self._kill_events_label)
		form.addRow("Casts", self._cast_events_label)
		form.addRow("Last", self._last_event_label)
		layout.addLayout(form)

		layout.addStretch(1)
		return panel

	def _build_center_tabs(self) -> QWidget:
		tabs = QTabWidget()
		tabs.addTab(self._build_event_table(), "Combat Events")
		tabs.addTab(self._build_perception_table(), "Agent Perception")
		return tabs

	def _build_event_table(self) -> QWidget:
		panel = QFrame()
		panel.setObjectName("panel")
		layout = QVBoxLayout(panel)

		title = QLabel("Combat Events")
		title.setObjectName("panelTitle")
		layout.addWidget(title)

		self._table = QTableWidget(0, 8)
		self._table.setHorizontalHeaderLabels(["Seq", "Time", "Type", "Source", "Target", "Amount", "HP", "Label"])
		self._table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
		self._table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
		self._table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
		self._table.verticalHeader().setVisible(False)
		self._table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
		self._table.horizontalHeader().setSectionResizeMode(7, QHeaderView.ResizeMode.Stretch)
		self._table.itemSelectionChanged.connect(self._show_selected_event)
		layout.addWidget(self._table, 1)

		return panel

	def _build_perception_table(self) -> QWidget:
		panel = QFrame()
		panel.setObjectName("panel")
		layout = QVBoxLayout(panel)

		header = QHBoxLayout()
		title = QLabel("Agent Perception")
		title.setObjectName("panelTitle")
		self._perception_agent_combo = QComboBox()
		self._perception_agent_combo.setMinimumWidth(260)
		self._perception_agent_combo.currentIndexChanged.connect(self._on_perception_agent_filter_changed)
		header.addWidget(title)
		header.addStretch(1)
		header.addWidget(QLabel("Agent"))
		header.addWidget(self._perception_agent_combo)
		layout.addLayout(header)

		self._perception_focus_label = QLabel("No agent selected")
		self._perception_focus_label.setObjectName("hintLabel")
		layout.addWidget(self._perception_focus_label)

		self._perception_table = QTableWidget(0, 18)
		self._perception_table.setHorizontalHeaderLabels([
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
		self._perception_table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
		self._perception_table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
		self._perception_table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
		self._perception_table.verticalHeader().setVisible(False)
		self._perception_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
		self._perception_table.horizontalHeader().setSectionResizeMode(17, QHeaderView.ResizeMode.Stretch)
		self._perception_table.itemSelectionChanged.connect(self._show_selected_perception)

		splitter = QSplitter(Qt.Orientation.Vertical, panel)
		splitter.addWidget(self._perception_table)
		splitter.addWidget(self._build_perception_detail_tabs())
		splitter.setSizes([220, 420])
		layout.addWidget(splitter, 1)

		return panel

	def _build_perception_detail_tabs(self) -> QWidget:
		tabs = QTabWidget()

		self._observed_entities_table = self._make_readonly_table([
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

		self._lost_entities_table = self._make_readonly_table([
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
		self._perception_combat_table = self._make_readonly_table([
			"Time",
			"Direction",
			"Type",
			"Source",
			"Target",
			"Amount",
			"HP",
			"Label",
		])
		combat_layout.addWidget(self._perception_combat_table, 1)
		tabs.addTab(combat_panel, "Combat")

		self._pings_table = self._make_readonly_table([
			"Memory",
			"Type",
			"Source",
			"Age",
			"TTL",
			"Position",
			"Active",
		])
		tabs.addTab(self._pings_table, "Pings")

		self._abilities_table = self._make_readonly_table([
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

	def _make_readonly_table(self, headers: list[str]) -> QTableWidget:
		table = QTableWidget(0, len(headers))
		table.setHorizontalHeaderLabels(headers)
		table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
		table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
		table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
		table.verticalHeader().setVisible(False)
		table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
		if headers:
			table.horizontalHeader().setSectionResizeMode(len(headers) - 1, QHeaderView.ResizeMode.Stretch)
		return table

	def _build_details_panel(self) -> QWidget:
		panel = QFrame()
		panel.setObjectName("panel")
		layout = QVBoxLayout(panel)

		title = QLabel("Event Details")
		title.setObjectName("panelTitle")
		layout.addWidget(title)

		self._details = QPlainTextEdit()
		self._details.setReadOnly(True)
		self._details.setFont(QFont("Consolas", 9))
		layout.addWidget(self._details, 1)

		return panel

	def _apply_style(self) -> None:
		self.setStyleSheet(
			"""
			QWidget {
				background: #111318;
				color: #e7eaf0;
				font-size: 13px;
			}
			QFrame#panel {
				background: #171a21;
				border: 1px solid #2a2f3a;
				border-radius: 6px;
			}
			QLabel#panelTitle {
				font-weight: 700;
				font-size: 14px;
				color: #f2f4f8;
			}
			QLabel#statusLabel {
				color: #9ad29a;
				font-weight: 700;
			}
			QLabel#errorLabel {
				color: #e2b36d;
			}
			QLabel#hintLabel {
				color: #a9b3c7;
			}
			QLineEdit, QSpinBox {
				background: #0d0f14;
				border: 1px solid #303745;
				border-radius: 4px;
				padding: 4px 6px;
			}
			QPushButton {
				background: #263248;
				border: 1px solid #3b4a66;
				border-radius: 4px;
				padding: 5px 10px;
			}
			QPushButton:hover {
				background: #31405c;
			}
			QTableWidget, QPlainTextEdit {
				background: #0d0f14;
				border: 1px solid #2a2f3a;
				gridline-color: #252a34;
				selection-background-color: #2e5b8f;
			}
			QHeaderView::section {
				background: #202633;
				color: #f2f4f8;
				border: 0;
				border-right: 1px solid #313746;
				padding: 5px;
			}
			"""
		)

	def _reconnect(self) -> None:
		self._error_label.setText("")
		self._client.stop()
		self._client.start(self._host_input.text().strip() or "127.0.0.1", self._port_input.value())

	def _clear_events(self) -> None:
		self._events.clear()
		self._event_counts.clear()
		self._perception_rows.clear()
		self._perception_snapshots.clear()
		self._perception_last_seen.clear()
		self._selected_perception_agent_id = 0
		self._focused_perception_agent_id = 0
		self._table.setRowCount(0)
		self._refresh_perception_agent_combo()
		self._refresh_perception_table()
		self._refresh_perception_details()
		self._details.clear()
		self._refresh_metrics()

	def _handle_telemetry_message(self, message: dict[str, Any]) -> None:
		schema = message.get("schema")
		if schema == "nawia.telemetry.combat.v1":
			self._append_event(message)
		elif schema == "nawia.telemetry.agent_perception.v1":
			self._upsert_perception(message)
		else:
			self._error_label.setText(f"Unknown telemetry schema: {schema}")

	def _append_event(self, event: dict[str, Any]) -> None:
		if len(self._events) >= MAX_EVENTS:
			self._events.pop(0)
			self._table.removeRow(0)

		self._events.append(event)
		event_type = str(event.get("event_type", "Unknown"))
		self._event_counts[event_type] += 1

		row = self._table.rowCount()
		self._table.insertRow(row)
		values = [
			event.get("sequence_id", ""),
			f"{float(event.get('time_seconds', 0.0)):.2f}",
			event_type,
			self._entity_name(event.get("source")),
			self._entity_name(event.get("target")),
			event.get("amount", ""),
			self._hp_text(event),
			event.get("source_label", ""),
		]

		for column, value in enumerate(values):
			item = QTableWidgetItem(str(value))
			if column in (0, 1, 5):
				item.setTextAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
			self._table.setItem(row, column, item)

		self._table.scrollToBottom()
		self._refresh_metrics()

	def _upsert_perception(self, snapshot: dict[str, Any]) -> None:
		agent = snapshot.get("agent")
		if not isinstance(agent, dict):
			return

		entity_id = self._entity_id(agent)
		if entity_id == 0:
			return

		self._perception_snapshots[entity_id] = snapshot
		self._perception_last_seen[entity_id] = monotonic()
		self._refresh_perception_agent_combo()
		self._refresh_perception_table()
		self._refresh_perception_details()

	def _refresh_perception_agent_combo(self) -> None:
		if not hasattr(self, "_perception_agent_combo"):
			return

		current_id = self._selected_perception_agent_id
		self._perception_agent_combo.blockSignals(True)
		self._perception_agent_combo.clear()
		self._perception_agent_combo.addItem("All agents", 0)
		for entity_id, snapshot in self._sorted_perception_snapshots():
			self._perception_agent_combo.addItem(self._entity_label(snapshot.get("agent")), entity_id)

		index = self._perception_agent_combo.findData(current_id)
		if index < 0:
			current_id = 0
			self._selected_perception_agent_id = 0
			index = 0
		self._perception_agent_combo.setCurrentIndex(index)
		self._perception_agent_combo.blockSignals(False)

	def _on_perception_agent_filter_changed(self, index: int) -> None:
		if index < 0:
			return

		value = self._perception_agent_combo.itemData(index)
		self._selected_perception_agent_id = int(value or 0)
		if self._selected_perception_agent_id:
			self._focused_perception_agent_id = self._selected_perception_agent_id
		self._refresh_perception_table()
		self._refresh_perception_details()

	def _refresh_perception_table(self) -> None:
		visible_snapshots = [
			(entity_id, snapshot)
			for entity_id, snapshot in self._sorted_perception_snapshots()
			if not self._selected_perception_agent_id or entity_id == self._selected_perception_agent_id
		]
		visible_ids = {entity_id for entity_id, _ in visible_snapshots}
		if self._selected_perception_agent_id in visible_ids:
			self._focused_perception_agent_id = self._selected_perception_agent_id
		elif self._focused_perception_agent_id not in visible_ids:
			self._focused_perception_agent_id = visible_snapshots[0][0] if visible_snapshots else 0

		self._perception_rows.clear()
		self._perception_table.blockSignals(True)
		self._perception_table.setRowCount(0)
		focused_row = -1
		for entity_id, snapshot in visible_snapshots:
			row = self._perception_table.rowCount()
			self._perception_table.insertRow(row)
			self._perception_rows[entity_id] = row
			self._populate_perception_row(row, snapshot)
			if self._focused_perception_agent_id == entity_id:
				focused_row = row
		self._perception_table.blockSignals(False)

		if focused_row >= 0:
			self._perception_table.selectRow(focused_row)
		else:
			self._perception_table.clearSelection()

	def _sorted_perception_snapshots(self) -> list[tuple[int, dict[str, Any]]]:
		return sorted(
			self._perception_snapshots.items(),
			key=lambda item: self._entity_label(item[1].get("agent")),
		)

	def _populate_perception_row(self, row: int, snapshot: dict[str, Any]) -> None:
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
			self._entity_label(agent),
			f"{agent.get('hp', '')}/{agent.get('max_hp', '')}",
			"yes" if agent.get("visible") else "no",
			self._interaction_text(agent),
			self._entity_label(current_target),
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
			item = self._perception_table.item(row, column)
			if item is None:
				item = QTableWidgetItem()
				self._perception_table.setItem(row, column, item)
			item.setText(str(value))
			if column in (0, 1, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16):
				item.setTextAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)

	def _refresh_perception_details(self) -> None:
		if not hasattr(self, "_observed_entities_table"):
			return

		snapshot = self._perception_snapshots.get(self._focused_perception_agent_id)
		if not snapshot:
			self._perception_focus_label.setText("No agent selected")
			self._incoming_damage_label.setText("Incoming damage: -")
			self._set_table_rows(self._observed_entities_table, [])
			self._set_table_rows(self._lost_entities_table, [])
			self._set_table_rows(self._perception_combat_table, [])
			self._set_table_rows(self._pings_table, [])
			self._set_table_rows(self._abilities_table, [])
			return

		agent = snapshot.get("agent")
		agent_id = self._entity_id(agent)
		current_target = snapshot.get("current_target")
		last_damage_source = snapshot.get("last_damage_source")
		self._perception_focus_label.setText(
			"Focus: "
			f"{self._entity_label(agent)} | "
			f"Target: {self._entity_label(current_target)} | "
			f"Last damage source: {self._entity_label(last_damage_source)} | "
			f"Lost memory: {self._float_text(snapshot.get('lost_memory_seconds'))}s"
		)

		observed_rows: list[list[Any]] = []
		for observed in snapshot.get("observed_entities", []):
			if not isinstance(observed, dict):
				continue

			entity = observed.get("entity")
			observed_rows.append([
				self._entity_name(entity),
				self._entity_field(entity, "entity_type"),
				self._entity_field(entity, "faction"),
				observed.get("relation", "-"),
				self._entity_hp(entity),
				self._float_text(observed.get("distance")),
				self._position_text(entity.get("position") if isinstance(entity, dict) else None),
				self._entity_flags(entity),
				self._interaction_text(entity),
				"yes" if observed.get("is_current_target") else "",
			])
		self._set_table_rows(self._observed_entities_table, observed_rows, {5})

		lost_rows: list[list[Any]] = []
		for lost in snapshot.get("lost_entities", []):
			if not isinstance(lost, dict):
				continue

			entity = lost.get("last_known_entity")
			lost_rows.append([
				self._entity_name(entity),
				self._entity_field(entity, "entity_type"),
				self._entity_field(entity, "faction"),
				lost.get("relation", "-"),
				self._position_text(lost.get("last_known_position")),
				f"{self._float_text(lost.get('seconds_since_seen'))}s",
				lost.get("disappearance_reason", "-"),
				"yes" if lost.get("was_current_target") else "",
				self._interaction_text(entity),
			])
		self._set_table_rows(self._lost_entities_table, lost_rows, {5})

		combat_events = [
			event for event in snapshot.get("recent_combat_events", [])
			if isinstance(event, dict)
		]
		incoming_events = [
			event for event in combat_events
			if event.get("event_type") == "DamageDealt" and self._entity_id(event.get("target")) == agent_id
		]
		if incoming_events:
			last_incoming = incoming_events[-1]
			self._incoming_damage_label.setText(
				"Incoming damage: "
				f"{self._entity_label(last_incoming.get('source'))} hit for {last_incoming.get('amount', '-')} "
				f"({last_incoming.get('source_label') or '-'}) "
				f"HP {self._hp_text(last_incoming)}"
			)
		else:
			self._incoming_damage_label.setText("Incoming damage: none in recent perception window")

		combat_rows: list[list[Any]] = []
		for event in reversed(combat_events):
			combat_rows.append([
				f"{float(event.get('time_seconds', 0.0)):.2f}",
				self._event_direction(event, agent_id),
				event.get("event_type", "-"),
				self._entity_label(event.get("source")),
				self._entity_label(event.get("target")),
				event.get("amount", ""),
				self._hp_text(event),
				event.get("source_label", ""),
			])
		self._set_table_rows(self._perception_combat_table, combat_rows, {0, 5})

		ping_rows: list[list[Any]] = []
		for memory_label, key in (("Visible", "visible_pings"), ("Remembered", "remembered_pings")):
			for ping in snapshot.get(key, []):
				if not isinstance(ping, dict):
					continue

				age = float(ping.get("age_seconds", 0.0))
				duration = float(ping.get("duration_seconds", 0.0))
				ttl = max(0.0, duration - age)
				ping_rows.append([
					memory_label,
					ping.get("ping_type", "-"),
					self._entity_label(ping.get("source")),
					f"{age:.2f}s",
					f"{ttl:.2f}s",
					self._position_text(ping.get("position")),
					"yes" if ping.get("active") else "no",
				])
		self._set_table_rows(self._pings_table, ping_rows, {3, 4})

		ability_rows: list[list[Any]] = []
		for ability in snapshot.get("abilities", []):
			if not isinstance(ability, dict):
				continue

			ability_rows.append([
				ability.get("slot", ""),
				ability.get("name", "-"),
				ability.get("target_type", "-"),
				"yes" if ability.get("ready") else "no",
				"yes" if ability.get("can_cast") else "no",
				self._cooldown_text(ability),
				self._float_text(ability.get("cast_range")),
				ability.get("damage", ""),
			])
		self._set_table_rows(self._abilities_table, ability_rows, {0, 5, 6, 7})

	def _set_table_rows(
		self,
		table: QTableWidget,
		rows: list[list[Any]],
		right_aligned_columns: set[int] | None = None,
	) -> None:
		right_aligned_columns = right_aligned_columns or set()
		table.setRowCount(0)
		for row_index, values in enumerate(rows):
			table.insertRow(row_index)
			for column, value in enumerate(values):
				item = QTableWidgetItem(str(value if value not in (None, "") else "-"))
				if column in right_aligned_columns:
					item.setTextAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
				table.setItem(row_index, column, item)

	def _remove_stale_perception_rows(self) -> None:
		now = monotonic()
		stale_ids = [
			entity_id
			for entity_id, last_seen in self._perception_last_seen.items()
			if now - last_seen > PERCEPTION_STALE_SECONDS
		]
		if not stale_ids:
			return

		for entity_id in stale_ids:
			self._perception_rows.pop(entity_id, None)
			self._perception_snapshots.pop(entity_id, None)
			self._perception_last_seen.pop(entity_id, None)

		if self._selected_perception_agent_id in stale_ids:
			self._selected_perception_agent_id = 0
		if self._focused_perception_agent_id in stale_ids:
			self._focused_perception_agent_id = 0
		self._refresh_perception_agent_combo()
		self._refresh_perception_table()
		self._refresh_perception_details()

	def _remove_perception_row(self, entity_id: int) -> None:
		self._perception_rows.pop(entity_id, None)
		self._perception_snapshots.pop(entity_id, None)
		self._perception_last_seen.pop(entity_id, None)
		if self._selected_perception_agent_id == entity_id:
			self._selected_perception_agent_id = 0
		if self._focused_perception_agent_id == entity_id:
			self._focused_perception_agent_id = 0
		self._refresh_perception_agent_combo()
		self._refresh_perception_table()
		self._refresh_perception_details()

	def _refresh_metrics(self) -> None:
		self._total_events_label.setText(str(len(self._events)))
		self._damage_events_label.setText(str(self._event_counts["DamageDealt"]))
		self._kill_events_label.setText(str(self._event_counts["EntityKilled"]))
		self._cast_events_label.setText(str(self._event_counts["AbilityCastStarted"]))
		self._last_event_label.setText(str(self._events[-1].get("event_type", "-")) if self._events else "-")

	def _show_selected_event(self) -> None:
		rows = self._table.selectionModel().selectedRows()
		if not rows:
			return

		row = rows[0].row()
		if 0 <= row < len(self._events):
			self._details.setPlainText(json.dumps(self._events[row], indent=2, ensure_ascii=False))

	def _show_selected_perception(self) -> None:
		rows = self._perception_table.selectionModel().selectedRows()
		if not rows:
			return

		row = rows[0].row()
		for entity_id, stored_row in self._perception_rows.items():
			if stored_row == row:
				snapshot = self._perception_snapshots.get(entity_id)
				if snapshot:
					self._focused_perception_agent_id = entity_id
					self._refresh_perception_details()
					self._details.setPlainText(json.dumps(snapshot, indent=2, ensure_ascii=False))
				return

	def _set_connected(self, connected: bool) -> None:
		if connected:
			self._status_label.setText("Connected")
			self._error_label.setText("")
		else:
			self._status_label.setText("Disconnected")

	def _status_label_text(self, text: str) -> None:
		self._status_label.setText(text)

	def _error_label_text(self, text: str) -> None:
		self._error_label.setText(text)

	def _entity_id(self, value: Any) -> int:
		if not isinstance(value, dict):
			return 0
		return int(value.get("entity_id") or value.get("runtime_id") or 0)

	def _entity_field(self, value: Any, field: str) -> str:
		if not isinstance(value, dict) or not value.get("valid"):
			return "-"
		return str(value.get(field) or "-")

	def _entity_name(self, value: Any) -> str:
		if not isinstance(value, dict) or not value.get("valid"):
			return "-"
		name = str(value.get("name") or value.get("entity_type") or "-")
		entity_id = self._entity_id(value)
		return f"{name}#{entity_id}" if entity_id else name

	def _entity_label(self, value: Any) -> str:
		if not isinstance(value, dict) or not value.get("valid"):
			return "-"

		name = self._entity_name(value)
		entity_type = str(value.get("entity_type") or "?")
		faction = str(value.get("faction") or "?")
		return f"{name} ({entity_type}/{faction})"

	def _entity_hp(self, value: Any) -> str:
		if not isinstance(value, dict) or not value.get("valid"):
			return "-"
		return f"{value.get('hp', '')}/{value.get('max_hp', '')}"

	def _entity_flags(self, value: Any) -> str:
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
			flags.append(f"rooted {self._float_text(value.get('root_remaining'))}s")
		if value.get("poisoned"):
			flags.append(f"poisoned {self._float_text(value.get('poison_remaining'))}s")
		if value.get("casting"):
			cast_name = str(value.get("cast_name") or "cast")
			flags.append(f"casting {cast_name} {self._float_text(value.get('cast_remaining'))}s")
		if value.get("hazard"):
			phase = str(value.get("hazard_phase") or "Hazard").lower()
			radius = self._float_text(value.get("hazard_radius"))
			if value.get("hazard_expanding_wave"):
				current_radius = self._float_text(value.get("hazard_current_radius"))
				radius = f"{current_radius}/{radius}"
			damage = value.get("hazard_damage_per_tick", 0)
			effect = " knockdown" if value.get("hazard_knock_down_player_on_hit") else ""
			if phase == "warning":
				kind = "wave warning" if value.get("hazard_expanding_wave") else "hazard warning"
				flags.append(f"{kind} {self._float_text(value.get('hazard_time_to_activate'))}s r{radius}{effect}")
			else:
				kind = f"wave {phase}" if value.get("hazard_expanding_wave") else f"hazard {phase}"
				flags.append(f"{kind} {self._float_text(value.get('hazard_remaining'))}s r{radius} dmg {damage}{effect}")
		return ", ".join(flags) if flags else "-"

	def _interaction_text(self, value: Any) -> str:
		if not isinstance(value, dict) or not value.get("interactable"):
			return "-"

		state = str(value.get("interaction_state") or "Interactable")
		available = "yes" if value.get("interaction_available") else "no"
		return f"{state}/{available}"

	def _event_direction(self, event: dict[str, Any], agent_id: int) -> str:
		source_id = self._entity_id(event.get("source"))
		target_id = self._entity_id(event.get("target"))
		if target_id == agent_id and source_id == agent_id:
			return "self"
		if target_id == agent_id:
			return "incoming"
		if source_id == agent_id:
			return "outgoing"
		return "nearby"

	def _position_text(self, value: Any) -> str:
		if not isinstance(value, dict):
			return "-"
		x = self._float_text(value.get("x"))
		y = self._float_text(value.get("y"))
		z = value.get("z")
		if z is None:
			return f"{x}, {y}"
		return f"{x}, {y}, {self._float_text(z)}"

	def _float_text(self, value: Any) -> str:
		try:
			return f"{float(value):.2f}"
		except (TypeError, ValueError):
			return "-"

	def _cooldown_text(self, ability: dict[str, Any]) -> str:
		remaining = self._float_text(ability.get("cooldown_remaining"))
		cooldown = self._float_text(ability.get("cooldown"))
		return f"{remaining}/{cooldown}"

	def _hp_text(self, event: dict[str, Any]) -> str:
		if event.get("event_type") != "DamageDealt":
			return ""
		return f"{event.get('hp_before', '')}->{event.get('hp_after', '')}"


def center_on_screen(window: QWidget) -> None:
	screen = QApplication.primaryScreen()
	if screen is None:
		return

	available = screen.availableGeometry()
	frame = window.frameGeometry()
	frame.moveCenter(available.center())
	window.move(frame.topLeft())
