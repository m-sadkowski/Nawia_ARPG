import json
from collections import Counter
from typing import Any

from PyQt6.QtCore import Qt
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
	QTableWidget,
	QTableWidgetItem,
	QVBoxLayout,
	QWidget,
)

from .telemetry_client import TelemetryClient


MAX_EVENTS = 1000


class MainWindow(QMainWindow):
	def __init__(self) -> None:
		super().__init__()
		self.setWindowTitle("Nawia ET Monitor")
		self.resize(1280, 720)
		self.setMinimumSize(960, 540)

		self._events: list[dict[str, Any]] = []
		self._event_counts: Counter[str] = Counter()
		self._client = TelemetryClient(parent=self)
		self._client.event_received.connect(self._append_event)
		self._client.connection_changed.connect(self._set_connected)
		self._client.status_changed.connect(self._status_label_text)
		self._client.error_received.connect(self._error_label_text)

		self._build_ui()
		self._apply_style()
		self._client.start()

	def _build_ui(self) -> None:
		root = QWidget(self)
		layout = QVBoxLayout(root)
		layout.setContentsMargins(10, 10, 10, 10)
		layout.setSpacing(8)

		layout.addLayout(self._build_toolbar())

		splitter = QSplitter(Qt.Orientation.Horizontal, root)
		splitter.addWidget(self._build_metrics_panel())
		splitter.addWidget(self._build_event_table())
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
		self._table.setRowCount(0)
		self._details.clear()
		self._refresh_metrics()

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
