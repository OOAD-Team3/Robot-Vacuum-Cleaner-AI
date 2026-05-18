#!/usr/bin/env python3
"""Process/TCP system test runner for rvc_app."""

from __future__ import annotations

import argparse
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Sequence, Tuple


DEFAULT_CASES_DIR = Path(__file__).resolve().parent / "cases"


class CaseError(RuntimeError):
    pass


@dataclass
class CaseStep:
    line_number: int
    directive: str
    value: str


@dataclass
class CaseResult:
    path: Path
    passed: bool
    message: str = ""


class TcpLineClient:
    def __init__(self, host: str, port: int, timeout: float) -> None:
        self._socket = socket.create_connection((host, port), timeout=timeout)
        self._socket.settimeout(timeout)
        self._file = self._socket.makefile("rw", newline="\n")

    def send(self, command: str) -> str:
        self._file.write(command + "\n")
        self._file.flush()
        response = self._file.readline()
        if response == "":
            raise CaseError("server closed the connection before sending a response")
        return response.rstrip("\r\n")

    def close(self) -> None:
        try:
            self._file.close()
        finally:
            self._socket.close()


def parse_case(path: Path) -> List[CaseStep]:
    steps: List[CaseStep] = []
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        stripped = raw_line.strip()
        if not stripped or stripped.startswith("#"):
            continue

        if raw_line == "SEND":
            steps.append(CaseStep(line_number, "SEND", ""))
            continue

        if stripped == "SEND_SPACES":
            steps.append(CaseStep(line_number, "SEND", "   "))
            continue

        for directive in ("SEND_RAW ", "SEND ", "EXPECT_CONTAINS ", "EXPECT "):
            if raw_line.startswith(directive):
                name = directive.strip()
                value = raw_line[len(directive) :]
                if name == "SEND_RAW":
                    name = "SEND"
                steps.append(CaseStep(line_number, name, value))
                break
        else:
            raise CaseError(f"{path}:{line_number}: unknown directive")

    if not steps:
        raise CaseError(f"{path}: empty case")

    return steps


def read_process_output(process: subprocess.Popen) -> str:
    try:
        stdout, stderr = process.communicate(timeout=0.2)
    except subprocess.TimeoutExpired:
        return ""

    output = []
    if stdout:
        output.append(stdout.strip())
    if stderr:
        output.append(stderr.strip())
    return "\n".join(part for part in output if part)


def wait_for_server(host: str, port: int, process: subprocess.Popen, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    last_error: Optional[BaseException] = None

    while time.monotonic() < deadline:
        if process.poll() is not None:
            output = read_process_output(process)
            detail = f":\n{output}" if output else ""
            raise RuntimeError(f"rvc_app exited early with code {process.returncode}{detail}")

        try:
            with socket.create_connection((host, port), timeout=0.2):
                return
        except OSError as error:
            last_error = error
            time.sleep(0.05)

    raise RuntimeError(f"timed out waiting for {host}:{port}: {last_error}")


def run_case(path: Path, host: str, port: int, timeout: float) -> CaseResult:
    last_response: Optional[str] = None
    client = TcpLineClient(host, port, timeout)

    try:
        for step in parse_case(path):
            if step.directive == "SEND":
                last_response = client.send(step.value)
                continue

            if last_response is None:
                raise CaseError(f"{path}:{step.line_number}: expectation before SEND")

            if step.directive == "EXPECT":
                if last_response != step.value:
                    raise CaseError(
                        f"{path}:{step.line_number}: expected exactly {step.value!r}, got {last_response!r}")
                continue

            if step.directive == "EXPECT_CONTAINS":
                if step.value not in last_response:
                    raise CaseError(
                        f"{path}:{step.line_number}: expected {last_response!r} to contain {step.value!r}")
                continue

            raise CaseError(f"{path}:{step.line_number}: unsupported directive {step.directive}")
    except (OSError, socket.timeout, CaseError) as error:
        return CaseResult(path, False, str(error))
    finally:
        client.close()

    return CaseResult(path, True)


def discover_cases(cases_dir: Path) -> List[Path]:
    return sorted(cases_dir.glob("*.rvcst"))


def start_app(app: str, host: str, port: int) -> subprocess.Popen:
    return subprocess.Popen(
        [app, "--host", host, "--port", str(port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.DEVNULL,
        text=True,
    )


def stop_app(process: subprocess.Popen) -> None:
    if process.poll() is not None:
        return

    process.terminate()
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=3)


def run_all(args: argparse.Namespace) -> int:
    cases_dir = Path(args.cases_dir)
    cases = discover_cases(cases_dir)
    if not cases:
        print(f"No .rvcst files found in {cases_dir}", file=sys.stderr)
        return 2

    process = start_app(args.app, args.host, args.port)
    try:
        wait_for_server(args.host, args.port, process, args.startup_timeout)

        results = [run_case(path, args.host, args.port, args.timeout) for path in cases]
    finally:
        stop_app(process)

    passed = sum(1 for result in results if result.passed)
    failed = len(results) - passed

    for result in results:
        status = "PASS" if result.passed else "FAIL"
        print(f"{status} {result.path.name}")
        if result.message:
            print(f"  {result.message}")

    print(f"\n{passed}/{len(results)} cases passed")
    return 0 if failed == 0 else 1


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Run rvc_app TCP system tests")
    parser.add_argument("--app", required=True, help="Path to rvc_app executable")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", default=18765, type=int)
    parser.add_argument("--cases-dir", default=str(DEFAULT_CASES_DIR))
    parser.add_argument("--timeout", default=2.0, type=float)
    parser.add_argument("--startup-timeout", default=5.0, type=float)
    args = parser.parse_args(argv)

    return run_all(args)


if __name__ == "__main__":
    raise SystemExit(main())
