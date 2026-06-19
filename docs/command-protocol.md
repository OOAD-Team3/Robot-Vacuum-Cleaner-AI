# RVC App Command Protocol

`rvc_app` exposes the existing RVC SW Controller through a line-based TCP command interface for a Python simulator or system test client.

The protocol does not use JSON. Each request is one line ending with `\n`, and each response is one line ending with `\n`.

This protocol satisfies the simulator command interface and controller state query requirements described by FR-023, FR-024, and UC-008.

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
SET_SENSOR_SNAPSHOT FRONT=0|1 BACK=0|1|UNKNOWN DUST=0|1
DUST_DETECTED
QUIT
```

Compatibility commands retained for older simulator/test clients:

```text
SET_LEFT 0|1
SET_OBSTACLES FRONT=0|1 BACK=0|1|UNKNOWN LEFT=0|1
POWER_TIMEOUT
```

`SET_SENSOR_SNAPSHOT` is the preferred command for new scenarios because it carries dust and obstacle inputs in one judgment cycle. `POWER_TIMEOUT` is retained only as a compatibility command and returns `ERR INVALID_STATE` unless a future policy starts a timer.

## Responses

Successful command responses:

```text
OK PONG
OK RESET
OK SET_FRONT
OK SET_BACK
OK SET_SENSOR_SNAPSHOT
OK SET_LEFT
OK SET_OBSTACLES
OK DUST_DETECTED
OK POWER_TIMEOUT
OK BYE
```

State response:

```text
OK STATE MOVEMENT=CLEANING DIRECTION=FORWARD ROTATION_ACTIVE=0 FRONT=0 BACK=UNKNOWN DUST=0 DRIVE=MOVE_FORWARD CLEANING_POWER=NORMAL
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
| `MOVEMENT` | `CLEANING`, `ROTATING` |
| `DIRECTION` | `FORWARD`, `BACKWARD` |
| `ROTATION_ACTIVE` | `0`, `1` |
| `FRONT` | `0`, `1` |
| `BACK` | `0`, `1`, `UNKNOWN` |
| `DUST` | `0`, `1`; latest dust sensor observation |
| `DRIVE` | `NONE`, `MOVE_FORWARD`, `MOVE_BACKWARD`, `TURN_CLOCKWISE_90`, `TURN_COUNTER_CLOCKWISE_90` |
| `CLEANING_POWER` | `OFF`, `NORMAL`, `BOOST` |
