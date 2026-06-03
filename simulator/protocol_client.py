"""TCP client and parser for the rvc_app line-based protocol."""

from __future__ import annotations

import socket
from typing import Dict, Optional


STATE_FIELDS = {
    "MOVEMENT",
    "FRONT",
    "BACK",
    "LEFT",
    "DUST",
    "DRIVE",
    "CLEANING_POWER",
    "TIMER_ACTIVE",
}


class ProtocolError(RuntimeError):
    pass


def parse_state_response(response: str) -> Dict[str, str]:
    """Parse an OK STATE key-value response without JSON."""
    parts = response.strip().split()
    if len(parts) < 3 or parts[0] != "OK" or parts[1] != "STATE":
        raise ProtocolError("response is not an OK STATE line")

    state: Dict[str, str] = {}
    for token in parts[2:]:
        if "=" not in token:
            raise ProtocolError(f"invalid state token: {token}")
        key, value = token.split("=", 1)
        if not key or value == "":
            raise ProtocolError(f"invalid state token: {token}")
        state[key] = value

    missing = STATE_FIELDS.difference(state.keys())
    if missing:
        missing_text = ", ".join(sorted(missing))
        raise ProtocolError(f"missing state field(s): {missing_text}")

    return state


class RvcClient:
    def __init__(self, host: str, port: int, timeout: float = 2.0) -> None:
        self._host = host
        self._port = port
        self._timeout = timeout
        self._socket: Optional[socket.socket] = None
        self._file = None

    @property
    def connected(self) -> bool:
        return self._socket is not None

    def connect(self) -> None:
        self.close()
        sock = socket.create_connection((self._host, self._port), timeout=self._timeout)
        sock.settimeout(self._timeout)
        self._socket = sock
        self._file = sock.makefile("rw", newline="\n")

    def close(self) -> None:
        if self._file is not None:
            try:
                self._file.close()
            except OSError:
                pass
            self._file = None

        if self._socket is not None:
            try:
                self._socket.close()
            except OSError:
                pass
            self._socket = None

    def send_command(self, command: str) -> str:
        if self._file is None:
            raise ConnectionError("not connected")

        try:
            self._file.write(command + "\n")
            self._file.flush()
            response = self._file.readline()
        except socket.timeout as error:
            self.close()
            raise TimeoutError("response timeout") from error
        except OSError as error:
            self.close()
            raise ConnectionError(str(error)) from error

        if response == "":
            self.close()
            raise ConnectionError("server closed the connection")

        return response.rstrip("\r\n")

    def get_state(self) -> Dict[str, str]:
        return parse_state_response(self.send_command("GET_STATE"))
