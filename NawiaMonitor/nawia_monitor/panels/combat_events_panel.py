from collections import Counter
from typing import Any, Callable

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import QFrame, QLabel, QTableWidgetItem, QVBoxLayout, QWidget

from .. import telemetry_format as fmt
from ..table_utils import make_readonly_table


MAX_EVENTS = 1000


class CombatEventsPanel(QFrame):
	def __init__(self, show_details: Callable[[dict[str, Any]], None], parent: QWidget | None = None) -> None:
		super().__init__(parent)
		self.setObjectName("panel")
		self._show_details = show_details
		self._events: list[dict[str, Any]] = []
		self._event_counts: Counter[str] = Counter()

		layout = QVBoxLayout(self)
		title = QLabel("Combat Events")
		title.setObjectName("panelTitle")
		layout.addWidget(title)

		self._table = make_readonly_table(["Seq", "Time", "Type", "Source", "Target", "Amount", "HP", "Label"])
		self._table.itemSelectionChanged.connect(self._show_selected_event)
		layout.addWidget(self._table, 1)

	def clear(self) -> None:
		self._events.clear()
		self._event_counts.clear()
		self._table.setRowCount(0)

	def append_event(self, event: dict[str, Any]) -> None:
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
			fmt.entity_name(event.get("source")),
			fmt.entity_name(event.get("target")),
			event.get("amount", ""),
			fmt.hp_text(event),
			event.get("source_label", ""),
		]

		for column, value in enumerate(values):
			item = QTableWidgetItem(str(value))
			if column in (0, 1, 5):
				item.setTextAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
			self._table.setItem(row, column, item)

		self._table.scrollToBottom()

	@property
	def total_events(self) -> int:
		return len(self._events)

	@property
	def event_counts(self) -> Counter[str]:
		return self._event_counts

	@property
	def last_event_type(self) -> str:
		return str(self._events[-1].get("event_type", "-")) if self._events else "-"

	def _show_selected_event(self) -> None:
		rows = self._table.selectionModel().selectedRows()
		if not rows:
			return

		row = rows[0].row()
		if 0 <= row < len(self._events):
			self._show_details(self._events[row])
