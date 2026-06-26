import sys

from PyQt6.QtWidgets import QApplication

from .main_window import MainWindow, center_on_screen


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
