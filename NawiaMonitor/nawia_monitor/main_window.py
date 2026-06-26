import json
from collections import Counter
from time import monotonic
from typing import Any

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import (
	QAbstractItemView,
	QApplication,
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

		title = QLabel("Agent Perception")
		title.setObjectName("panelTitle")
		layout.addWidget(title)

		self._perception_table = QTableWidget(0, 17)
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
			"Events",
			"Ready Abilities",
		])
		self._perception_table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
		self._perception_table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
		self._perception_table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
		self._perception_table.verticalHeader().setVisible(False)
		self._perception_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
		self._perception_table.horizontalHeader().setSectionResizeMode(16, QHeaderView.ResizeMode.Stretch)
		self._perception_table.itemSelectionChanged.connect(self._show_selected_perception)
		layout.addWidget(self._perception_table, 1)

		return panel

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
		self._table.setRowCount(0)
		self._perception_table.setRowCount(0)
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

		runtime_id = int(agent.get("runtime_id") or 0)
		if runtime_id == 0:
			return

		row = self._perception_rows.get(runtime_id)
		if row is None:
			row = self._perception_table.rowCount()
			self._perception_table.insertRow(row)
			self._perception_rows[runtime_id] = row

		self._perception_snapshots[runtime_id] = snapshot
		self._perception_last_seen[runtime_id] = monotonic()
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
			len(snapshot.get("recent_combat_events", [])),
			", ".join(ready_abilities) if ready_abilities else "-",
		]

		for column, value in enumerate(values):
			item = self._perception_table.item(row, column)
			if item is None:
				item = QTableWidgetItem()
				self._perception_table.setItem(row, column, item)
			item.setText(str(value))
			if column in (0, 1, 7, 8, 9, 10, 11, 12, 13, 14, 15):
				item.setTextAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)

	def _remove_stale_perception_rows(self) -> None:
		now = monotonic()
		stale_ids = [
			runtime_id
			for runtime_id, last_seen in self._perception_last_seen.items()
			if now - last_seen > PERCEPTION_STALE_SECONDS
		]
		for runtime_id in stale_ids:
			self._remove_perception_row(runtime_id)

	def _remove_perception_row(self, runtime_id: int) -> None:
		row = self._perception_rows.get(runtime_id)
		if row is None:
			return

		self._perception_table.removeRow(row)
		self._perception_rows.pop(runtime_id, None)
		self._perception_snapshots.pop(runtime_id, None)
		self._perception_last_seen.pop(runtime_id, None)

		for other_id, other_row in list(self._perception_rows.items()):
			if other_row > row:
				self._perception_rows[other_id] = other_row - 1

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
		for runtime_id, stored_row in self._perception_rows.items():
			if stored_row == row:
				snapshot = self._perception_snapshots.get(runtime_id)
				if snapshot:
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

	def _entity_name(self, value: Any) -> str:
		if not isinstance(value, dict) or not value.get("valid"):
			return "-"
		return str(value.get("name") or value.get("entity_type") or "-")

	def _entity_label(self, value: Any) -> str:
		if not isinstance(value, dict) or not value.get("valid"):
			return "-"

		name = str(value.get("name") or value.get("entity_type") or "-")
		entity_type = str(value.get("entity_type") or "?")
		faction = str(value.get("faction") or "?")
		return f"{name} ({entity_type}/{faction})"

	def _interaction_text(self, value: Any) -> str:
		if not isinstance(value, dict) or not value.get("interactable"):
			return "-"

		state = str(value.get("interaction_state") or "Interactable")
		available = "yes" if value.get("interaction_available") else "no"
		return f"{state}/{available}"

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
