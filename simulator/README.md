# RVC Pygame Simulator

This simulator is an external TCP client for `rvc_app`. The C++ `RVCSWController` still owns the robot decision logic. Python simulates the environment, renders the screen, calculates sensor inputs from a grid map, sends line-based commands, and applies the C++ `DRIVE` result as simple physics.

No JSON is used. Every request and response is one newline-terminated line.

## Install Python Dependencies

Use a virtual environment on Homebrew Python:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r simulator/requirements.txt
```

Windows:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r simulator\requirements.txt
```

## Build And Run rvc_app

macOS:

```sh
cmake -S . -B build
cmake --build build
./build/rvc_app --listen 127.0.0.1:9090
```

Windows:

```powershell
cmake -S . -B build
cmake --build build --config Debug
.\build\Debug\rvc_app.exe --listen 127.0.0.1:9090
```

Windows may show a firewall prompt the first time the app listens on a TCP port.

## Run Map Simulation Mode

Map mode is the default. The Python world stores walls, dust, cleaned cells, robot position, and robot direction. Each simulation step sends `SET_SENSOR_SNAPSHOT FRONT=... BACK=... DUST=...` based on the robot's neighboring cells and current tile, then reads `GET_STATE` and applies the returned `DRIVE` value.

Coverage assist is available for debugging. It opens the neighboring cell or cells that lead by the shortest reachable path to the nearest uncleaned floor target, and reports less useful neighboring cells as temporary virtual obstacles. This keeps the demo from orbiting a local wall while another reachable area remains uncleaned, while C++ still decides the actual `DRIVE` command from the sensor line it receives. Press `B` to toggle this assist.

Dust remains on the map after Boost cleaning. When the robot returns to the same dust tile, the simulator reports `DUST=1` again so the controller can run the Boost rotation again.

macOS:

```sh
python simulator/simulator.py --host 127.0.0.1 --port 9090 --mode map
```

Windows:

```powershell
python simulator\simulator.py --host 127.0.0.1 --port 9090 --mode map
```

Use a custom map:

```sh
python simulator/simulator.py --host 127.0.0.1 --port 9090 --map simulator/maps/basic.txt
```

The simulator also includes built-in map presets for system-test-like situations:

| Preset | Purpose |
| --- | --- |
| `basic` | Default room with an inner obstacle and dust |
| `open_room` | Larger open area for coverage behavior |
| `front_obstacle` | Starts with a front obstacle condition |
| `three_side_back_clear` | Starts with front/left/right blocked and back available |
| `deep_dead_end` | Winding room that includes deeper three-sided dead ends |
| `busy_room` | More dust and internal obstacles |
| `obstacle_dense` | Denser obstacle layout with several dust points |

Press `N` in map mode to switch to the next preset map, or `V` to switch to the previous preset. Switching maps resets the Python world and sends `RESET` to the C++ controller so both sides start from a clean state.

Map characters:

```text
# = wall
. = floor
D = dust
R = robot initial position
```

## Run Manual Sensor Mode

Manual mode keeps the original sensor-command behavior. It is useful for debugging the TCP protocol directly.

```sh
python simulator/simulator.py --host 127.0.0.1 --port 9090 --mode manual
```

## Map Mode Keyboard Controls

| Key | Action |
| --- | --- |
| `Space` | Run one simulation step |
| `Enter` | Toggle play/pause |
| `R` | Reset Python world and C++ controller |
| `G` | Send `GET_STATE` |
| `P` | Send `PING` |
| `M` | Toggle manual/map mode |
| `C` | Restore dust from the initial map |
| `B` | Toggle coverage assist |
| `N` | Switch to next built-in map preset |
| `V` | Switch to previous built-in map preset |
| `Esc` | Exit |

In play mode, the simulator runs one step about every 0.45 seconds.

Dust tiles are not removed during play mode, and reaching the inner area does not stop the simulator automatically.

## Manual Mode Keyboard Controls

| Key | Command |
| --- | --- |
| `G` | `GET_STATE` |
| `P` | `PING` |
| `R` | `RESET`, then `GET_STATE` |
| `M` | Toggle map/manual mode |
| `1` | `SET_FRONT 0`, then `GET_STATE` |
| `2` | `SET_FRONT 1`, then `GET_STATE` |
| `3` | `SET_BACK 0`, then `GET_STATE` |
| `4` | `SET_BACK 1`, then `GET_STATE` |
| `5` | `SET_BACK UNKNOWN`, then `GET_STATE` |
| `A` | `SET_SENSOR_SNAPSHOT FRONT=0 BACK=0 DUST=0`, then `GET_STATE` |
| `S` | `SET_SENSOR_SNAPSHOT FRONT=1 BACK=0 DUST=0`, then `GET_STATE` |
| `D` | `SET_SENSOR_SNAPSHOT FRONT=0 BACK=1 DUST=0`, then `GET_STATE` |
| `F` | `SET_SENSOR_SNAPSHOT FRONT=1 BACK=1 DUST=0`, then `GET_STATE` |
| `Z` | `SET_SENSOR_SNAPSHOT FRONT=0 BACK=0 DUST=1`, then `GET_STATE` |
| `X` | `SET_SENSOR_SNAPSHOT FRONT=0 BACK=0 DUST=0`, then `GET_STATE` |
| `Esc` | Exit |

## GET_STATE Parsing

The simulator expects a single-line key-value response:

```text
OK STATE MOVEMENT=CLEANING DIRECTION=FORWARD ROTATION_ACTIVE=0 FRONT=0 BACK=UNKNOWN DUST=0 DRIVE=MOVE_FORWARD CLEANING_POWER=NORMAL
```

It parses the response by splitting whitespace and `KEY=VALUE` tokens. JSON is not used.

## Responsibility Split

- Python owns the map, robot coordinates, direction, dust locations, sensor calculation, and rendering.
- C++ owns the controller decision. Python does not choose whether to move forward, turn, stop, or move backward.
- Python applies `DRIVE=MOVE_FORWARD`, `MOVE_BACKWARD`, `TURN_CLOCKWISE_90`, `TURN_COUNTER_CLOCKWISE_90`, `STOP`, or `NONE` to its local world after C++ returns `GET_STATE`.
- Coverage assist only changes the Python-side sensor values sent to C++; it does not change the C++ protocol or domain model.
