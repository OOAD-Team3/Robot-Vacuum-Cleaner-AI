#!/usr/bin/env sh
set -eu

APP_PATH="${1:-./build/rvc_app}"
HOST="${RVC_TEST_HOST:-127.0.0.1}"
PORT="${RVC_TEST_PORT:-18765}"

python3 system_tests/run_system_tests.py --app "$APP_PATH" --host "$HOST" --port "$PORT"
