import json
from typing import Optional

from PyQt6.QtCore import QObject, QTimer, pyqtSignal
from PyQt6.QtNetwork import QAbstractSocket, QTcpSocket


class TelemetryClient(QObject):
	event_received = pyqtSignal(dict)
	connection_changed = pyqtSignal(bool)
	status_changed = pyqtSignal(str)
	error_received = pyqtSignal(str)

	def __init__(
		self,
		host: str = "127.0.0.1",
		port: int = 19777,
		reconnect_ms: int = 1000,
		parent: Optional[QObject] = None,
	) -> None:
		super().__init__(parent)
		self._host = host
		self._port = port
		self._buffer = b""

		self._socket = QTcpSocket(self)
		self._socket.connected.connect(self._on_connected)
		self._socket.disconnected.connect(self._on_disconnected)
		self._socket.readyRead.connect(self._on_ready_read)
		self._socket.errorOccurred.connect(self._on_error)

		self._reconnect_timer = QTimer(self)
		self._reconnect_timer.setInterval(reconnect_ms)
		self._reconnect_timer.timeout.connect(self._connect_if_needed)

	def start(self, host: Optional[str] = None, port: Optional[int] = None) -> None:
		if host is not None:
			self._host = host
		if port is not None:
			self._port = port

		self._reconnect_timer.start()
		self._connect_if_needed()

	def stop(self) -> None:
		self._reconnect_timer.stop()
		self._socket.abort()
		self._buffer = b""
		self.connection_changed.emit(False)
		self.status_changed.emit("Disconnected")

	@property
	def host(self) -> str:
		return self._host

	@property
	def port(self) -> int:
		return self._port

	def _connect_if_needed(self) -> None:
		state = self._socket.state()
		if state in (
			QAbstractSocket.SocketState.ConnectedState,
			QAbstractSocket.SocketState.ConnectingState,
		):
			return

		self.status_changed.emit(f"Connecting to {self._host}:{self._port}")
		self._socket.abort()
		self._socket.connectToHost(self._host, self._port)

	def _on_connected(self) -> None:
		self._buffer = b""
		self.connection_changed.emit(True)
		self.status_changed.emit(f"Connected to {self._host}:{self._port}")

	def _on_disconnected(self) -> None:
		self.connection_changed.emit(False)
		self.status_changed.emit("Disconnected")

	def _on_error(self, _error: QAbstractSocket.SocketError) -> None:
		if self._socket.state() == QAbstractSocket.SocketState.ConnectedState:
			return
		self.error_received.emit(self._socket.errorString())

	def _on_ready_read(self) -> None:
		self._buffer += bytes(self._socket.readAll())

		while b"\n" in self._buffer:
			raw_line, self._buffer = self._buffer.split(b"\n", 1)
			line = raw_line.strip()
			if not line:
				continue

			try:
				event = json.loads(line.decode("utf-8"))
			except (UnicodeDecodeError, json.JSONDecodeError) as exc:
				self.error_received.emit(f"Invalid telemetry line: {exc}")
				continue

			if isinstance(event, dict):
				self.event_received.emit(event)
			else:
				self.error_received.emit("Telemetry payload is not a JSON object")
