import json
import sys
from typing import Any

from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import (
	QApplication,
	QFrame,
	QHBoxLayout,
	QLabel,
	QLineEdit,
	QMainWindow,
	QPlainTextEdit,
	QPushButton,
	QSpinBox,
	QSplitter,
	QTabWidget,
	QVBoxLayout,
	QWidget,
)
from PyQt6.QtCore import Qt

from .panels.agent_commands_panel import AgentCommandsPanel
from .panels.combat_events_panel import CombatEventsPanel
from .panels.metrics_panel import MetricsPanel
from .panels.perception_panel import PerceptionPanel
from .style import MONITOR_STYLESHEET
from .telemetry_client import TelemetryClient


class MainWindow(QMainWindow):
	def __init__(self) -> None:
		super().__init__()
		self.setWindowTitle("Nawia ET Monitor")
		self.resize(1280, 720)
		self.setMinimumSize(960, 540)

		self._client = TelemetryClient(parent=self)
		self._client.event_received.connect(self._handle_telemetry_message)
		self._client.connection_changed.connect(self._set_connected)
		self._client.status_changed.connect(self._status_label_text)
		self._client.error_received.connect(self._error_label_text)
		self._details_text: QPlainTextEdit | None = None

		self._build_ui()
		self.setStyleSheet(MONITOR_STYLESHEET)
		self._client.start()

	def _build_ui(self) -> None:
		root = QWidget(self)
		layout = QVBoxLayout(root)
		layout.setContentsMargins(10, 10, 10, 10)
		layout.setSpacing(8)

		layout.addLayout(self._build_toolbar())

		self._details = self._build_details_panel()
		self._combat_panel = CombatEventsPanel(self._show_details)
		self._perception_panel = PerceptionPanel(self._show_details)
		self._commands_panel = AgentCommandsPanel(self._show_details)
		self._metrics_panel = MetricsPanel()

		splitter = QSplitter(Qt.Orientation.Horizontal, root)
		splitter.addWidget(self._metrics_panel)
		splitter.addWidget(self._build_center_tabs())
		splitter.addWidget(self._details)
		splitter.setSizes([220, 720, 340])
		layout.addWidget(splitter, 1)

		self.setCentralWidget(root)
		self._refresh_metrics()

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

	def _build_center_tabs(self) -> QWidget:
		tabs = QTabWidget()
		tabs.addTab(self._combat_panel, "Combat Events")
		tabs.addTab(self._perception_panel, "Agent Perception")
		tabs.addTab(self._commands_panel, "Agent Commands")
		return tabs

	def _build_details_panel(self) -> QWidget:
		panel = QFrame()
		panel.setObjectName("panel")
		layout = QVBoxLayout(panel)

		title = QLabel("Event Details")
		title.setObjectName("panelTitle")
		layout.addWidget(title)

		self._details_text = QPlainTextEdit()
		self._details_text.setReadOnly(True)
		self._details_text.setFont(QFont("Consolas", 9))
		layout.addWidget(self._details_text, 1)
		return panel

	def _show_details(self, payload: dict[str, Any]) -> None:
		if self._details_text is not None:
			self._details_text.setPlainText(json.dumps(payload, indent=2, ensure_ascii=False))

	def _reconnect(self) -> None:
		self._error_label.setText("")
		self._client.stop()
		self._client.start(self._host_input.text().strip() or "127.0.0.1", self._port_input.value())

	def _clear_events(self) -> None:
		self._combat_panel.clear()
		self._perception_panel.clear()
		self._commands_panel.clear()
		if self._details_text is not None:
			self._details_text.clear()
		self._refresh_metrics()

	def _handle_telemetry_message(self, message: dict[str, Any]) -> None:
		schema = message.get("schema")
		if schema == "nawia.telemetry.combat.v1":
			self._combat_panel.append_event(message)
		elif schema == "nawia.telemetry.agent_perception.v1":
			self._perception_panel.upsert(message)
		elif schema == "nawia.telemetry.agent_command.v1":
			self._commands_panel.upsert(message)
		else:
			self._error_label.setText(f"Unknown telemetry schema: {schema}")
			return

		self._refresh_metrics()

	def _refresh_metrics(self) -> None:
		self._metrics_panel.refresh(
			self._combat_panel.total_events,
			self._combat_panel.event_counts,
			self._combat_panel.last_event_type,
			self._commands_panel.active_count,
			self._commands_panel.completed_count,
		)

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


def center_on_screen(window: QWidget) -> None:
	screen = QApplication.primaryScreen()
	if screen is None:
		return

	available = screen.availableGeometry()
	frame = window.frameGeometry()
	frame.moveCenter(available.center())
	window.move(frame.topLeft())


def main() -> int:
	app = QApplication(sys.argv)
	app.setApplicationName("Nawia ET Monitor")
	app.setOrganizationName("Nawia")

	window = MainWindow()
	center_on_screen(window)
	window.show()

	return app.exec()


if __name__ == "__main__":
	raise SystemExit(main())
