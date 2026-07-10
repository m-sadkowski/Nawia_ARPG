from collections import Counter

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import QFormLayout, QFrame, QLabel, QVBoxLayout, QWidget


class MetricsPanel(QFrame):
	def __init__(self, parent: QWidget | None = None) -> None:
		super().__init__(parent)
		self.setObjectName("panel")

		layout = QVBoxLayout(self)
		title = QLabel("Telemetry")
		title.setObjectName("panelTitle")
		layout.addWidget(title)

		form = QFormLayout()
		form.setLabelAlignment(Qt.AlignmentFlag.AlignLeft)
		self._total_events_label = QLabel("0")
		self._damage_events_label = QLabel("0")
		self._kill_events_label = QLabel("0")
		self._cast_events_label = QLabel("0")
		self._active_commands_label = QLabel("0")
		self._completed_commands_label = QLabel("0")
		self._last_event_label = QLabel("-")
		form.addRow("Events", self._total_events_label)
		form.addRow("Damage", self._damage_events_label)
		form.addRow("Kills", self._kill_events_label)
		form.addRow("Casts", self._cast_events_label)
		form.addRow("Active Cmds", self._active_commands_label)
		form.addRow("Done Cmds", self._completed_commands_label)
		form.addRow("Last", self._last_event_label)
		layout.addLayout(form)
		layout.addStretch(1)

	def refresh(
		self,
		total_events: int,
		event_counts: Counter[str],
		last_event_type: str,
		active_commands: int,
		completed_commands: int,
	) -> None:
		self._total_events_label.setText(str(total_events))
		self._damage_events_label.setText(str(event_counts["DamageDealt"]))
		self._kill_events_label.setText(str(event_counts["EntityKilled"]))
		self._cast_events_label.setText(str(event_counts["AbilityCastStarted"]))
		self._active_commands_label.setText(str(active_commands))
		self._completed_commands_label.setText(str(completed_commands))
		self._last_event_label.setText(last_event_type)
