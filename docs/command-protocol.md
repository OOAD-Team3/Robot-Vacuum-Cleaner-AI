# RVC App Command Protocol

`rvc_app` exposes the existing RVC SW Controller through a line-based TCP command interface for a Python simulator or system test client.

The protocol does not use JSON. Each request is one line ending with `\n`, and each response is one line ending with `\n`.

## Connection

Default endpoint:

```text
127.0.0.1:9090
```

The simulator should connect as a TCP client and send one command per line.
Command lines longer than 1024 bytes are rejected with `ERR INVALID_ARGUMENT`.

## Commands

```text
PING
RESET
GET_STATE
SET_FRONT 0|1
SET_BACK 0|1|UNKNOWN
SET_SIDE LEFT=0|1 RIGHT=0|1
SET_OBSTACLES FRONT=0|1 BACK=0|1|UNKNOWN LEFT=0|1 RIGHT=0|1
DUST_DETECTED
POWER_TIMEOUT
QUIT
```

## Responses

Successful command responses:

```text
OK PONG
OK RESET
OK SET_FRONT
OK SET_BACK
OK SET_SIDE
OK SET_OBSTACLES
OK DUST_DETECTED
OK POWER_TIMEOUT
OK BYE
```

State response:

```text
OK STATE MOVEMENT=CLEANING FRONT=0 BACK=UNKNOWN LEFT=0 RIGHT=0 DUST=0 DRIVE=MOVE_FORWARD CLEANING_POWER=NORMAL TIMER_ACTIVE=0
```

Error responses:

```text
ERR UNKNOWN_COMMAND
ERR INVALID_ARGUMENT
ERR INVALID_STATE
```

## State Fields

| Field | Values |
| --- | --- |
| `MOVEMENT` | `CLEANING`, `AVOIDING_OBSTACLE`, `BLOCKED`, `STOPPED` |
| `FRONT` | `0`, `1` |
| `BACK` | `0`, `1`, `UNKNOWN` |
| `LEFT` | `0`, `1` |
| `RIGHT` | `0`, `1` |
| `DUST` | `0`, `1`; external dust-response observation, set after `DUST_DETECTED` and cleared by `POWER_TIMEOUT` or `RESET` |
| `DRIVE` | `NONE`, `MOVE_FORWARD`, `MOVE_BACKWARD`, `TURN_LEFT`, `TURN_RIGHT`, `STOP` |
| `CLEANING_POWER` | `OFF`, `NORMAL`, `INCREASED` |
| `TIMER_ACTIVE` | `0`, `1` |
