from typing import Any, Callable

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import QFrame, QLabel, QTabWidget, QTableWidget, QTableWidgetItem, QVBoxLayout, QWidget

from ..table_utils import make_readonly_table


EMPTY_COMMAND_PAYLOAD: dict[str, Any] = {
	"schema": "nawia.telemetry.agent_command.v1",
	"active_commands": [],
	"completed_commands": [],
}


class AgentCommandsPanel(QFrame):
	def __init__(self, show_details: Callable[[dict[str, Any]], None], parent: QWidget | None = None) -> None:
		super().__init__(parent)
		self.setObjectName("panel")
		self._show_details = show_details
		self._payload = EMPTY_COMMAND_PAYLOAD.copy()

		layout = QVBoxLayout(self)
		title = QLabel("Agent Commands")
		title.setObjectName("panelTitle")
		layout.addWidget(title)

		tabs = QTabWidget()
		self._active_table = self._make_command_table()
		self._completed_table = self._make_command_table()
		tabs.addTab(self._active_table, "Active")
		tabs.addTab(self._completed_table, "Completed")
		layout.addWidget(tabs, 1)

	def clear(self) -> None:
		self._payload = EMPTY_COMMAND_PAYLOAD.copy()
		self._refresh_tables()

	def upsert(self, payload: dict[str, Any]) -> None:
		self._payload = payload
		self._refresh_tables()

	@property
	def active_count(self) -> int:
		active = self._payload.get("active_commands", [])
		return len(active) if isinstance(active, list) else 0

	@property
	def completed_count(self) -> int:
		completed = self._payload.get("completed_commands", [])
		return len(completed) if isinstance(completed, list) else 0

	def _make_command_table(self) -> QTableWidget:
		table = make_readonly_table([
			"ID",
			"Agent",
			"Type",
			"Status",
			"Reason",
			"Age",
			"Target",
			"Slot",
			"Path",
			"Message",
		])
		table.itemSelectionChanged.connect(lambda table=table: self._show_selected_command(table))
		return table

	def _refresh_tables(self) -> None:
		active_commands = [
			command for command in self._payload.get("active_commands", [])
			if isinstance(command, dict)
		]
		completed_commands = [
			command for command in self._payload.get("completed_commands", [])
			if isinstance(command, dict)
		]
		self._set_command_rows(self._active_table, active_commands)
		self._set_command_rows(self._completed_table, completed_commands)

	def _set_command_rows(self, table: QTableWidget, commands: list[dict[str, Any]]) -> None:
		table.setRowCount(0)
		for row_index, command in enumerate(commands):
			table.insertRow(row_index)
			request = command.get("request") if isinstance(command.get("request"), dict) else {}
			values = [
				command.get("command_id", ""),
				command.get("agent_entity_id", ""),
				command.get("type", "-"),
				command.get("status", "-"),
				command.get("failure_reason", "-"),
				f"{float(command.get('age_seconds', 0.0)):.2f}s",
				request.get("target_entity_id", "-"),
				request.get("ability_slot", "-"),
				f"{command.get('path_index', 0)}/{command.get('path_length', 0)}",
				command.get("message", "-"),
			]
			for column, value in enumerate(values):
				item = QTableWidgetItem(str(value if value not in (None, "") else "-"))
				if column == 0:
					item.setData(Qt.ItemDataRole.UserRole, command)
				if column in (0, 1, 5, 6, 7, 8):
					item.setTextAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
				table.setItem(row_index, column, item)

	def _show_selected_command(self, table: QTableWidget) -> None:
		rows = table.selectionModel().selectedRows()
		if not rows:
			return

		item = table.item(rows[0].row(), 0)
		if item is None:
			return

		command = item.data(Qt.ItemDataRole.UserRole)
		if isinstance(command, dict):
			self._show_details(command)
