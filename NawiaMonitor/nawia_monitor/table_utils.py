from typing import Any

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
	QAbstractItemView,
	QHeaderView,
	QTableWidget,
	QTableWidgetItem,
)


def make_readonly_table(headers: list[str]) -> QTableWidget:
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


def set_table_rows(
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
