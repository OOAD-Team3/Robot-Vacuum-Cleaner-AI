# Robot Vacuum Cleaner AI

Robot Vacuum Cleaner(RVC) SW Controller의 요구사항과 설계 문서, 코드를 관리하는 저장소입니다. 현재 범위는 자동 청소 기능이며, 센서/모터/청소 장치의 상세 하드웨어 구현은 추상화된 입력과 출력으로 다룹니다.

### Folder Structure

```text
.
├── README.md
├── CMakeLists.txt
├── include
│   └── rvc
│       ├── net
│       │   └── TcpServer.hpp
│       ├── protocol
│       │   ├── CommandHandler.hpp
│       │   ├── CommandParser.hpp
│       │   └── StateFormatter.hpp
│       ├── sim
│       │   ├── ControllerStateSnapshot.hpp
│       │   ├── RecordingDevices.hpp
│       │   └── RobotVacuumApplication.hpp
│       ├── AutomaticCleaning.hpp
│       ├── CleaningPolicy.hpp
│       ├── Commands.hpp
│       ├── Devices.hpp
│       ├── DustResponse.hpp
│       ├── RVCSWController.hpp
│       ├── SensorState.hpp
│       └── Types.hpp
├── src
│   ├── AutomaticCleaning.cpp
│   ├── CleaningPolicy.cpp
│   ├── Commands.cpp
│   ├── DustResponse.cpp
│   ├── RVCSWController.cpp
│   ├── SensorState.cpp
│   ├── net
│   │   └── TcpServer.cpp
│   ├── protocol
│   │   ├── CommandHandler.cpp
│   │   ├── CommandParser.cpp
│   │   └── StateFormatter.cpp
│   └── sim
│       ├── ControllerStateSnapshot.cpp
│       ├── RecordingDevices.cpp
│       └── RobotVacuumApplication.cpp
├── app
│   └── rvc_app
│       └── main.cpp
├── simulator
│   ├── __init__.py
│   ├── README.md
│   ├── maps
│   │   └── basic.txt
│   ├── protocol_client.py
│   ├── requirements.txt
│   ├── simulator.py
│   └── world.py
├── system_tests
│   ├── README.md
│   ├── run_system_tests.py
│   ├── run_system_tests.sh
│   ├── run_system_tests.ps1
│   ├── run_system_tests.bat
│   └── cases
│       └── *.rvcst
└── docs
    ├── requirements.md                              # 전체 범위, 요구사항, 제외 범위, 확장 후보
    ├── functional-and-non-functional-requirements.md # FR/NFR ID 기준 문서
    ├── command-protocol.md                         # rvc_app TCP command/response protocol
    ├── usecases.md                                 # UC-001~UC-007 상세 시나리오와 요구사항 추적
    ├── usecase-diagram.puml                        # 유스케이스와 외부 액터 관계
    ├── domain-model.puml                           # 도메인 개념과 관계
    ├── class-diagram.puml                          # 시퀀스 다이어그램에서 도출된 클래스 구조
    ├── ssd                                         # 시스템과 외부 액터 간 이벤트 흐름
    │   ├── UC-001.puml
    │   ├── UC-002.puml
    │   ├── UC-003.puml
    │   ├── UC-004.puml
    │   ├── UC-005.puml
    │   ├── UC-006.puml
    │   └── UC-007.puml
    └── sd                                          # 시스템 내부 객체 간 협력 흐름
        ├── SD-01.puml
        ├── SD-02.puml
        ├── SD-03.puml
        ├── SD-04.puml
        ├── SD-05.puml
        ├── SD-06.puml
        └── SD-07.puml
```

### Current Use Case Map

| Use Case | Summary                       | SSD                    | Internal SD          |
| -------- | ----------------------------- | ---------------------- | -------------------- |
| UC-001   | 장애물이 없을 때 직진 청소    | `docs/ssd/UC-001.puml` | `docs/sd/SD-01.puml` |
| UC-002   | 전방 장애물 감지 후 정지      | `docs/ssd/UC-002.puml` | `docs/sd/SD-02.puml` |
| UC-003   | 좌측 또는 우측 회피 방향 선택 | `docs/ssd/UC-003.puml` | `docs/sd/SD-03.puml` |
| UC-004   | 방향 전환 후 청소 재개        | `docs/ssd/UC-004.puml` | `docs/sd/SD-04.puml` |
| UC-005   | 삼면 장애물 감지 후 후진      | `docs/ssd/UC-005.puml` | `docs/sd/SD-05.puml` |
| UC-006   | 먼지 감지 이벤트 처리         | `docs/ssd/UC-006.puml` | `docs/sd/SD-06.puml` |
| UC-007   | 청소 출력 일반 상태 복귀      | `docs/ssd/UC-007.puml` | `docs/sd/SD-07.puml` |

### Implementation

The OOI implementation provides the RVC SW Controller as a C++17 library target named `rvc_controller`.

- `RVCSWController` receives sensor and time events, updates `SensorState`, and executes device calls.
- `AutomaticCleaning` owns the automatic cleaning, obstacle avoidance, and dust response decisions.
- `DrivingDevice`, `CleaningDevice`, and `Time` are abstract interfaces for hardware/time dependencies.
- `CommandResult`, `MovementCommand`, and `CleaningCommand` carry decisions from domain logic to the controller.
- UC-005 uses `backObstacleDetected` to distinguish backward-available and backward-unavailable flows.
- `rvc_app` adds a line-based TCP command server around the existing controller for simulator and system test clients.
- The TCP server uses standalone Asio through CMake `FetchContent`; the first configure needs network access unless the dependency has already been populated.

### Build

#### macOS

```sh
cmake -S . -B build
cmake --build build
```

Run unit tests:

```sh
ctest --test-dir build --output-on-failure
```

Run the app:

```sh
./build/rvc_app --listen 127.0.0.1:9090
./build/rvc_app --host 127.0.0.1 --port 9090
```

#### Windows

Visual Studio generator example:

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\rvc_app.exe --listen 127.0.0.1:9090
.\build\Debug\rvc_app.exe --host 127.0.0.1 --port 9090
```

Ninja generator example:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
.\build\rvc_app.exe --listen 127.0.0.1:9090
```

Windows may show a firewall prompt the first time `rvc_app` listens on a TCP port.

### Simulator Command Interface

By default, `rvc_app` listens on `127.0.0.1:9090`. A Python Pygame simulator can connect to that host and port as a TCP client and send one newline-terminated command at a time.

The protocol is plain command/response text, not JSON. See [`docs/command-protocol.md`](docs/command-protocol.md).

Example:

```text
PING
GET_STATE
SET_OBSTACLES FRONT=1 BACK=0 LEFT=1 RIGHT=1
DUST_DETECTED
POWER_TIMEOUT
QUIT
```

### Pygame Simulator

The simulator is a Python TCP client. In map mode, Python owns the room grid, dust positions, robot coordinates, robot direction, sensor calculation, and rendering. C++ still owns the controller decision logic: Python sends `SET_OBSTACLES`, reads `GET_STATE`, and applies the returned `DRIVE` value to the local world.

Map mode enables a Python-side coverage assist by default so the robot does not only follow the outer wall or orbit a local obstacle while reachable uncleaned cells remain elsewhere. The assist is implemented by sensor calculation only; the C++ command protocol and domain model are unchanged.

When the C++ state reports `TIMER_ACTIVE=1` after dust detection, the simulator automatically sends `POWER_TIMEOUT` after about five seconds so cleaning power returns from `INCREASED` to `NORMAL`. Use `--dust-timeout` if the controller policy duration changes.

When every floor cell reachable from the initial robot position has been cleaned, the simulator stops play mode automatically.

Map mode also includes built-in preset maps for protocol/system-test-like scenarios such as front obstacle, three-side obstacle with back available, deeper dead ends, and denser dust/obstacle layouts. Press `N`/`V` in the simulator to switch maps; the Python world is replaced and the C++ controller receives `RESET`.

Manual mode is also available for direct sensor-command debugging.

Install dependencies:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r simulator/requirements.txt
```

Run on macOS:

```sh
./build/rvc_app --listen 127.0.0.1:9090
python3 simulator/simulator.py --host 127.0.0.1 --port 9090 --mode map
```

Run on Windows:

```powershell
.\build\Debug\rvc_app.exe --listen 127.0.0.1:9090
python simulator\simulator.py --host 127.0.0.1 --port 9090 --mode map
```

Use `--mode manual` to keep the original keyboard-driven sensor input mode. See [`simulator/README.md`](simulator/README.md) for keyboard controls and map file format.

### System Tests

System tests run `rvc_app` as an external process and verify the TCP command/response protocol with Python standard library code. They are separate from the Google Test unit tests.

macOS:

```sh
python3 system_tests/run_system_tests.py --app ./build/rvc_app --host 127.0.0.1 --port 18765
```

Windows:

```powershell
python system_tests\run_system_tests.py --app .\build\Debug\rvc_app.exe --host 127.0.0.1 --port 18765
```

The current suite contains positive and negative `.rvcst` cases under [`system_tests/cases`](system_tests/cases). See [`system_tests/README.md`](system_tests/README.md).
