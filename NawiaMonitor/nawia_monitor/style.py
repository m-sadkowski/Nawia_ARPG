MONITOR_STYLESHEET = """
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
