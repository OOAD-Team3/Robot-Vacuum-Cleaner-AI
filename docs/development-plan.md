# Implementation Plan: Dust-First Bidirectional Cleaning

## Overview

이 계획은 변경된 요구사항을 현재 C++ RVC SW Controller에 반영하기 위한 구현 순서이다.

핵심 변경은 먼지 감지 우선 판단, 전진/후진 진행 방향 상태, 90도 단위 제자리 회전 재확인, 먼지 대응 중에만 Boost 모드 사용, 장애물 대응 중 일반 모드 유지이다. 기존 좌/우 회피 및 타이머 기반 먼지 대응 흐름은 신규 요구사항 기준으로 대체한다.

## Requirements

- 전원이 켜져 있으면 항상 청소 상태를 유지한다.
- 먼지 감지를 장애물 감지보다 먼저 처리한다.
- 먼지 감지 시에만 Boost 모드를 적용한다.
- 전진 중 먼지 또는 장애물을 만나면 시계 방향 90도 회전 후 후방 센서를 확인한다.
- 후진 중 먼지 또는 장애물을 만나면 반시계 방향 90도 회전 후 전방 센서를 확인한다.
- 대상 센서가 열릴 때까지 90도 단위 회전과 센서 확인을 반복한다.
- 회전 절차가 끝나면 진행 방향을 전진/후진 사이에서 Toggle한다.
- TCP 상태 조회는 진행 방향, 회전 상태, 청소 출력 모드를 관찰할 수 있어야 한다.

## Architecture Changes

- `include/rvc/Types.hpp`
  - `TravelDirection`, `RotationDirection`, `RotationCause`, `TargetSensor` 추가
  - `MovementCommandType`을 `TURN_CLOCKWISE_90`, `TURN_COUNTER_CLOCKWISE_90` 중심으로 재정의 또는 기존 `TurnRight`, `TurnLeft`에 의미 매핑
- `include/rvc/SensorState.hpp`, `src/SensorState.cpp`
  - 전방/후방 장애물과 먼지 감지를 하나의 snapshot으로 갱신하는 API 추가
  - `obstacleDetectedIn(TravelDirection)` 추가
- `include/rvc/AutomaticCleaning.hpp`, `src/AutomaticCleaning.cpp`
  - 기존 좌/우 회피, 우측 탐색, 삼면 장애물, 먼지 타이머 흐름 제거 또는 비활성화
  - 진행 방향과 회전 컨텍스트 기반 정책 추가
- `include/rvc/Commands.hpp`, `src/Commands.cpp`
  - 90도 회전 명령과 진행 방향 명령 생성 지원
- `include/rvc/Devices.hpp`, `src/sim/RecordingDevices.cpp`
  - 시계/반시계 90도 회전 명령을 명확히 기록하도록 정리
- `include/rvc/RVCSWController.hpp`, `src/RVCSWController.cpp`
  - sensor snapshot 입력과 회전 재확인 흐름을 적용
  - Boost 명령은 먼지 회전 절차에서만 실행
- `include/rvc/protocol/CommandParser.hpp`, `src/protocol/CommandParser.cpp`
  - 먼지와 장애물을 함께 전달하는 sensor snapshot command 추가 검토
- `include/rvc/protocol/StateFormatter.hpp`, `src/protocol/StateFormatter.cpp`
  - `DIRECTION`, `ROTATION_ACTIVE`, `CLEANING_POWER` 상태 출력 추가
- `tests/RVCSWControllerTest.cpp`
  - 신규 요구사항 중심 단위 테스트로 교체 또는 재구성
- `system_tests/cases/`
  - 먼지 우선순위, 전진/후진 Toggle, 반복 회전 시나리오 추가

## Implementation Steps

### Phase 1: Domain State Foundation

1. **진행 방향 타입 추가** (File: `include/rvc/Types.hpp`)
   - Action: `TravelDirection { Forward, Backward }`를 추가한다.
   - Why: 현재 진행 방향을 명시적으로 보관해야 먼지/장애물 처리 후 Toggle할 수 있다.
   - Dependencies: None
   - Risk: Low

2. **회전 컨텍스트 타입 추가** (Files: `include/rvc/AutomaticCleaning.hpp`, `src/AutomaticCleaning.cpp`)
   - Action: 회전 원인, 회전 방향, 확인 대상 센서, 다음 진행 방향을 저장하는 `RotationContext`를 추가한다.
   - Why: 90도 회전 후 센서 재확인 루프를 단일 상태로 관리하기 위함이다.
   - Dependencies: Phase 1 Step 1
   - Risk: Medium

3. **센서 snapshot API 추가** (Files: `include/rvc/SensorState.hpp`, `src/SensorState.cpp`)
   - Action: 전방/후방/먼지 상태를 함께 갱신하는 API와 `obstacleDetectedIn(direction)`을 추가한다.
   - Why: 먼지 우선 판단은 동일 판단 주기의 센서 조합을 기준으로 해야 한다.
   - Dependencies: Phase 1 Step 1
   - Risk: Medium

### Phase 2: Cleaning Policy Rewrite

1. **먼지 우선 판단 구현** (Files: `include/rvc/AutomaticCleaning.hpp`, `src/AutomaticCleaning.cpp`)
   - Action: `decideNextAction(sensorState)`에서 먼지 감지를 먼저 확인하고, 먼지가 없을 때만 현재 진행 방향 장애물을 확인한다.
   - Why: 신규 요구사항의 최상위 분기이다.
   - Dependencies: Phase 1
   - Risk: Medium

2. **먼지 회전 절차 구현** (Files: `include/rvc/AutomaticCleaning.hpp`, `src/AutomaticCleaning.cpp`)
   - Action: 전진 중 먼지는 Boost + 시계 90도 + 후방 확인, 후진 중 먼지는 Boost + 반시계 90도 + 전방 확인으로 시작한다.
   - Why: UC-003, UC-004의 핵심 동작이다.
   - Dependencies: Phase 2 Step 1
   - Risk: Medium

3. **장애물 회전 절차 구현** (Files: `include/rvc/AutomaticCleaning.hpp`, `src/AutomaticCleaning.cpp`)
   - Action: 전진 중 전방 장애물은 일반 모드 + 시계 90도 + 후방 확인, 후진 중 후방 장애물은 일반 모드 + 반시계 90도 + 전방 확인으로 시작한다.
   - Why: UC-005, UC-006의 핵심 동작이다.
   - Dependencies: Phase 2 Step 1
   - Risk: Medium

4. **회전 재확인 루프 구현** (Files: `include/rvc/AutomaticCleaning.hpp`, `src/AutomaticCleaning.cpp`)
   - Action: 대상 센서가 막혀 있으면 동일 방향 90도 회전 명령을 반복하고, 열리면 진행 방향을 Toggle한 뒤 주행 명령을 반환한다.
   - Why: UC-007을 단일 재사용 흐름으로 구현한다.
   - Dependencies: Phase 2 Step 2, Phase 2 Step 3
   - Risk: High

5. **타이머 기반 먼지 응답 제거 또는 격리** (Files: `include/rvc/DustResponse.hpp`, `src/DustResponse.cpp`, `include/rvc/RVCSWController.hpp`, `src/RVCSWController.cpp`)
   - Action: 먼지 Boost를 시간 기반이 아닌 회전 절차 기반으로 바꾼다.
   - Why: Boost는 먼지 감지로 시작된 제자리 회전 중에만 적용해야 한다.
   - Dependencies: Phase 2 Step 2, Phase 2 Step 4
   - Risk: High

### Phase 3: Controller and Adapter Integration

1. **Controller 입력 흐름 갱신** (Files: `include/rvc/RVCSWController.hpp`, `src/RVCSWController.cpp`)
   - Action: sensor snapshot 입력을 받아 `SensorState`를 갱신하고 `AutomaticCleaning`의 결과를 적용한다.
   - Why: 먼지/장애물 우선순위를 같은 판단 주기에서 처리하기 위함이다.
   - Dependencies: Phase 1, Phase 2
   - Risk: Medium

2. **Movement command 실행 갱신** (Files: `include/rvc/Devices.hpp`, `src/RVCSWController.cpp`, `src/sim/RecordingDevices.cpp`)
   - Action: 시계/반시계 90도 회전 명령을 주행 장치 호출로 연결한다.
   - Why: 요구사항이 좌/우 회피가 아닌 방향성 있는 90도 회전을 요구한다.
   - Dependencies: Phase 2
   - Risk: Low

3. **Protocol command 갱신** (Files: `docs/command-protocol.md`, `include/rvc/protocol/CommandParser.hpp`, `src/protocol/CommandParser.cpp`, `src/protocol/CommandHandler.cpp`)
   - Action: `SET_SENSOR_SNAPSHOT FRONT=0|1 BACK=0|1|UNKNOWN DUST=0|1` 형태의 통합 입력 명령을 추가한다.
   - Why: 먼지와 장애물이 동시에 감지되는 우선순위 시나리오를 정확히 검증하기 위함이다.
   - Dependencies: Phase 3 Step 1
   - Risk: Medium

4. **State snapshot 갱신** (Files: `include/rvc/sim/ControllerStateSnapshot.hpp`, `src/sim/ControllerStateSnapshot.cpp`, `src/protocol/StateFormatter.cpp`)
   - Action: 진행 방향, 회전 활성 여부, Boost/Normal 상태를 상태 응답에 포함한다.
   - Why: FR-024와 시스템 테스트 관찰 가능성을 만족한다.
   - Dependencies: Phase 3 Step 1
   - Risk: Low

### Phase 4: Tests and Verification

1. **Unit tests 작성** (File: `tests/RVCSWControllerTest.cpp`)
   - Action: 전진/후진 정상 주행, 먼지 우선순위, 먼지 회전, 장애물 회전, 반복 회전, Toggle을 테스트한다.
   - Why: 신규 동작의 대부분은 도메인 판단이므로 단위 테스트가 가장 빠른 피드백을 준다.
   - Dependencies: Phase 2, Phase 3
   - Risk: Medium

2. **System tests 작성** (Directory: `system_tests/cases/`)
   - Action: TCP command를 통해 먼지와 장애물 동시 입력, 후방 센서 반복 감지, 전방 센서 반복 감지, 상태 조회를 검증한다.
   - Why: rvc_app 경계에서 프로토콜과 컨트롤러 통합을 확인한다.
   - Dependencies: Phase 3 Step 3, Phase 3 Step 4
   - Risk: Medium

3. **문서 동기화 확인** (Files: `README.md`, `docs/*.md`, `docs/*.puml`)
   - Action: README의 Current Use Case Map과 command protocol이 최종 구현과 일치하는지 확인한다.
   - Why: 요구사항, 설계, 구현 사이의 추적성을 유지한다.
   - Dependencies: Phase 4 Step 1, Phase 4 Step 2
   - Risk: Low

## Testing Strategy

- Unit tests
  - `AutomaticCleaning`의 먼지 우선 판단
  - `RotationContext` 시작/반복/완료
  - 전진 중 먼지 감지 후 후진 전환
  - 후진 중 먼지 감지 후 전진 전환
  - 전진 중 장애물 감지 후 후진 전환
  - 후진 중 장애물 감지 후 전진 전환
  - 먼지와 장애물 동시 감지 시 먼지 우선 처리
- Integration tests
  - `RVCSWController`가 `DrivingDevice`, `CleaningDevice`에 올바른 명령 순서를 전달하는지 검증
  - sensor snapshot command가 도메인 판단으로 전달되는지 검증
- System tests
  - TCP 명령으로 `SET_SENSOR_SNAPSHOT` 후 `GET_STATE` 확인
  - 반복 회전 후 대상 센서가 열릴 때 진행 방향이 Toggle되는지 확인
  - 먼지 회전 중에는 `CLEANING_POWER=BOOST`, 완료 후 `NORMAL`인지 확인

## Risks & Mitigations

- **Risk**: 기존 테스트가 좌/우 회피와 먼지 타이머 흐름에 강하게 결합되어 실패할 수 있다.
  - Mitigation: 기존 요구사항을 대체하는 테스트를 먼저 작성하고, 더 이상 유효하지 않은 테스트는 새 유스케이스 기준으로 제거 또는 개명한다.
- **Risk**: 기존 TCP protocol은 먼지와 장애물을 같은 판단 주기에 전달하기 어렵다.
  - Mitigation: 통합 sensor snapshot command를 추가하고 기존 명령은 호환용으로 유지한다.
- **Risk**: 회전 반복이 무한히 지속될 수 있다.
  - Mitigation: 현재 요구사항에서는 범위 밖으로 문서화하고, 향후 최대 회전 횟수 또는 정지 정책을 확장 후보로 유지한다.
- **Risk**: `TurnLeft/TurnRight`와 시계/반시계 용어가 혼재할 수 있다.
  - Mitigation: 도메인 명령은 `TURN_CLOCKWISE_90`, `TURN_COUNTER_CLOCKWISE_90`로 명명하고, 하드웨어 어댑터에서 기존 좌/우 호출에 매핑한다.

## Success Criteria

- [x] `docs/requirements.md`, `docs/usecases.md`, SSD, SD, class diagram이 동일한 유스케이스와 요구사항을 가리킨다.
- [x] 전원 ON 중 기본 상태가 일반 모드 청소로 유지된다.
- [x] 먼지와 장애물이 동시에 감지되면 먼지 대응 절차가 먼저 실행된다.
- [x] 전진 중 먼지 감지 시 Boost + 시계 90도 회전 + 후방 확인 + 후진 전환이 수행된다.
- [x] 후진 중 먼지 감지 시 Boost + 반시계 90도 회전 + 전방 확인 + 전진 전환이 수행된다.
- [x] 전진 중 장애물 감지 시 일반 모드 + 시계 90도 회전 + 후방 확인 + 후진 전환이 수행된다.
- [x] 후진 중 장애물 감지 시 일반 모드 + 반시계 90도 회전 + 전방 확인 + 전진 전환이 수행된다.
- [x] 대상 센서가 막혀 있으면 같은 방향 90도 회전과 센서 재확인이 반복된다.
- [x] 상태 조회에서 진행 방향, 회전 상태, 청소 출력 모드를 확인할 수 있다.
- [x] 단위 테스트와 시스템 테스트가 통과한다.
