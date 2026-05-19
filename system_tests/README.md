# RVC System Tests

These system tests run `rvc_app` as an external process and verify the TCP command/response protocol. They do not use Google Test and do not call C++ domain classes directly.

## Run

macOS:

```sh
python3 system_tests/run_system_tests.py --app ./build/rvc_app --host 127.0.0.1 --port 18765
```

Windows:

```powershell
python system_tests\run_system_tests.py --app .\build\Debug\rvc_app.exe --host 127.0.0.1 --port 18765
```

Convenience wrappers:

```sh
system_tests/run_system_tests.sh ./build/rvc_app
```

```powershell
.\system_tests\run_system_tests.ps1 -App .\build\Debug\rvc_app.exe
```

The runner starts `rvc_app`, waits until the TCP port accepts connections, runs every `.rvcst` file under `system_tests/cases`, and terminates `rvc_app` at the end.

If `rvc_app` exits before the TCP port opens, the runner prints the captured app stdout/stderr so startup failures such as bind or permission errors are visible.

## Case Format

Each case file is plain text. Blank lines and lines starting with `#` are ignored.

```text
SEND PING
EXPECT OK PONG

SEND GET_STATE
EXPECT_CONTAINS OK STATE
EXPECT_CONTAINS MOVEMENT=
```

Directives:

| Directive | Meaning |
| --- | --- |
| `SEND <command>` | Send one newline-terminated command. |
| `SEND` | Send an empty command line. |
| `SEND_SPACES` | Send a line containing spaces only. |
| `SEND_RAW <text>` | Send raw text after the directive. |
| `EXPECT <text>` | Require the previous response to exactly match text. |
| `EXPECT_CONTAINS <text>` | Require the previous response to contain text. |

## Cases

Current case count: 34.

Positive cases:

- `positive_01_ping.rvcst`
- `positive_02_get_state_initial.rvcst`
- `positive_03_set_front_clear.rvcst`
- `positive_04_set_front_blocked.rvcst`
- `positive_05_set_back_clear.rvcst`
- `positive_06_set_back_blocked.rvcst`
- `positive_07_set_back_unknown.rvcst`
- `positive_08_set_side_clear.rvcst`
- `positive_09_set_side_left_blocked_avoidance.rvcst`
- `positive_10_set_side_right_blocked_avoidance.rvcst`
- `positive_11_obstacles_all_clear.rvcst`
- `positive_12_obstacles_front_blocked.rvcst`
- `positive_13_three_side_back_available.rvcst`
- `positive_14_all_blocked.rvcst`
- `positive_15_dust_detected_response.rvcst`
- `positive_16_dust_increases_power.rvcst`
- `positive_17_power_timeout_response.rvcst`
- `positive_18_power_timeout_restores_normal.rvcst`
- `positive_19_reset_restores_initial_state.rvcst`
- `positive_20_multiple_commands_server_stays_alive.rvcst`
- `positive_21_quit.rvcst`
- `positive_22_lowercase_ping.rvcst`

Negative cases:

- `negative_01_unknown_command.rvcst`
- `negative_02_empty_command.rvcst`
- `negative_03_spaces_only_command.rvcst`
- `negative_04_set_front_missing_argument.rvcst`
- `negative_05_set_front_invalid_value.rvcst`
- `negative_06_set_back_invalid_value.rvcst`
- `negative_07_set_side_missing_argument.rvcst`
- `negative_08_set_side_invalid_key.rvcst`
- `negative_09_set_obstacles_missing_argument.rvcst`
- `negative_10_set_obstacles_invalid_key.rvcst`
- `negative_11_too_long_unknown_command.rvcst`
- `negative_12_power_timeout_without_timer.rvcst`
