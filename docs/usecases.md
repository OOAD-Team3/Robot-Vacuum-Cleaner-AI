# RVC SW Controller Use Cases

## 1. Purpose

이 문서는 `requirements.md`와 `functional-and-non-functional-requirements.md`를 바탕으로 Robot Vacuum Cleaner(RVC) SW Controller의 use case를 정리한다. 이때 Use Case Summary 및 Traceability Matrix에서 사용하는 `FR-`/`NFR-` 형태의 요구사항 ID는 `functional-and-non-functional-requirements.md`를 기준으로 한다.

현재 범위는 전원이 켜진 동안 자동 청소 상태를 유지하는 RVC의 주행 방향, 센서 판단 우선순위, 먼지/장애물 대응 회전, 청소 출력 모드 제어이다. 센서, 모터, 브러시의 상세 하드웨어 구현은 제외하고, SW Controller가 입력을 해석해 어떤 동작 명령을 결정해야 하는지에 집중한다.

## 2. Actors

| Actor | Description |
| --- | --- |
| 센서 입력 | 전방/후방 장애물 감지와 먼지 감지 상태를 SW Controller에 제공하는 추상화된 입력이다. |
| 주행 장치 | 전진, 후진, 시계 방향 90도 회전, 반시계 방향 90도 회전 명령을 수행하는 추상화된 출력 대상이다. |
| 청소 장치 | 일반 모드 또는 Boost 모드로 청소를 수행하는 추상화된 출력 대상이다. |
| 시뮬레이터/시스템 테스트 클라이언트 | TCP 명령 인터페이스를 통해 센서 입력, 초기화, 상태 조회, 종료 명령을 전달하는 외부 클라이언트이다. |

Python 시뮬레이터와 시스템 테스트 클라이언트는 TCP 명령 인터페이스를 통해 위의 추상화된 센서 입력을 구체적으로 전달하고 상태를 조회한다. 이는 자동 청소 use case를 실행하기 위한 외부 검증/시연 채널이며, UC-008로 별도 표현한다.

## 3. Use Case Summary

| ID | Use Case | Primary Actor | Related Requirements |
| --- | --- | --- | --- |
| UC-001 | 전원 ON 중 일반 모드 청소 유지 | 센서 입력 | FR-001, FR-002, FR-003, FR-005, FR-022 |
| UC-002 | 먼지 우선 센서 판단 | 센서 입력 | FR-006, FR-007 |
| UC-003 | 전진 중 먼지 감지 후 후진 전환 | 센서 입력 | FR-008, FR-009, FR-010, FR-020, FR-021 |
| UC-004 | 후진 중 먼지 감지 후 전진 전환 | 센서 입력 | FR-011, FR-012, FR-013, FR-020, FR-021 |
| UC-005 | 전진 중 장애물 감지 후 후진 전환 | 센서 입력 | FR-014, FR-015, FR-016, FR-020 |
| UC-006 | 후진 중 장애물 감지 후 전진 전환 | 센서 입력 | FR-004, FR-017, FR-018, FR-019, FR-020 |
| UC-007 | 90도 단위 회전 중 센서 재확인 | 센서 입력 | FR-009, FR-012, FR-015, FR-018, FR-020 |
| UC-008 | TCP 명령 인터페이스로 컨트롤러 구동 및 상태 조회 | 시뮬레이터/시스템 테스트 클라이언트 | FR-023, FR-024, NFR-008 |

## 4. Use Cases

### UC-001. 전원 ON 중 일반 모드 청소 유지

| Item | Description |
| --- | --- |
| Preconditions | RVC의 전원이 켜져 있다. |
| Main Scenario | 1. SW Controller는 현재 진행 방향을 확인한다.<br>2. 센서 입력에서 먼지 감지 상태를 확인한다.<br>3. 먼지가 감지되지 않으면 현재 진행 방향의 장애물 감지 상태를 확인한다.<br>4. 먼지와 현재 진행 방향의 장애물이 모두 감지되지 않으면 청소 장치에 일반 모드 명령을 전달한다.<br>5. 주행 장치에 현재 진행 방향 유지 명령을 전달한다. |
| Alternative/Exception Scenarios | A1. 먼지가 감지되면 UC-002를 거쳐 현재 진행 방향에 따라 UC-003 또는 UC-004로 전환한다.<br>A2. 먼지가 감지되지 않고 현재 진행 방향의 장애물이 감지되면 UC-002를 거쳐 현재 진행 방향에 따라 UC-005 또는 UC-006으로 전환한다. |
| Result | RVC는 전원이 켜진 동안 일반 모드로 현재 진행 방향 청소를 계속한다. |

### UC-002. 먼지 우선 센서 판단

| Item | Description |
| --- | --- |
| Preconditions | RVC가 자동 청소 중이며 센서 입력을 수신할 수 있다. |
| Main Scenario | 1. SW Controller는 먼지 감지 여부를 먼저 확인한다.<br>2. 먼지가 감지되면 장애물 감지 여부와 관계없이 먼지 대응 절차를 선택한다.<br>3. 먼지가 감지되지 않으면 현재 진행 방향의 장애물 감지 여부를 확인한다.<br>4. 장애물이 감지되면 장애물 대응 절차를 선택한다.<br>5. 먼지와 장애물이 모두 감지되지 않으면 UC-001을 유지한다. |
| Alternative/Exception Scenarios | A1. 먼지와 장애물이 동시에 감지되면 먼지 대응 절차를 우선한다.<br>A2. 현재 진행 방향이 전진이면 전방 장애물을 확인한다.<br>A3. 현재 진행 방향이 후진이면 후방 장애물을 확인한다. |
| Result | SW Controller는 먼지 감지, 장애물 감지, 정상 주행 중 하나의 절차를 결정한다. |

### UC-003. 전진 중 먼지 감지 후 후진 전환

| Item | Description |
| --- | --- |
| Preconditions | 현재 진행 방향은 전진이고 먼지가 감지되었다. |
| Main Scenario | 1. SW Controller는 청소 장치에 Boost 모드 명령을 전달한다.<br>2. 주행 장치에 시계 방향 90도 제자리 회전 명령을 전달한다.<br>3. 후방 센서 상태를 확인한다.<br>4. 후방 장애물이 감지되지 않으면 청소 장치에 일반 모드 명령을 전달한다.<br>5. 현재 진행 방향을 후진으로 Toggle한다.<br>6. 주행 장치에 후진 명령을 전달한다. |
| Alternative/Exception Scenarios | A1. 최초 90도 회전 후 후방 장애물이 감지되면 UC-007을 수행한다.<br>A2. 회전 중에도 Boost 모드를 유지한다. |
| Result | RVC는 먼지 구간을 Boost 모드로 제자리 회전 청소한 뒤 후진 방향으로 청소를 계속한다. |

### UC-004. 후진 중 먼지 감지 후 전진 전환

| Item | Description |
| --- | --- |
| Preconditions | 현재 진행 방향은 후진이고 먼지가 감지되었다. |
| Main Scenario | 1. SW Controller는 청소 장치에 Boost 모드 명령을 전달한다.<br>2. 주행 장치에 반시계 방향 90도 제자리 회전 명령을 전달한다.<br>3. 전방 센서 상태를 확인한다.<br>4. 전방 장애물이 감지되지 않으면 청소 장치에 일반 모드 명령을 전달한다.<br>5. 현재 진행 방향을 전진으로 Toggle한다.<br>6. 주행 장치에 전진 명령을 전달한다. |
| Alternative/Exception Scenarios | A1. 최초 90도 회전 후 전방 장애물이 감지되면 UC-007을 수행한다.<br>A2. 회전 중에도 Boost 모드를 유지한다. |
| Result | RVC는 먼지 구간을 Boost 모드로 제자리 회전 청소한 뒤 전진 방향으로 청소를 계속한다. |

### UC-005. 전진 중 장애물 감지 후 후진 전환

| Item | Description |
| --- | --- |
| Preconditions | 현재 진행 방향은 전진이고, 먼지는 감지되지 않았으며, 전방 장애물이 감지되었다. |
| Main Scenario | 1. SW Controller는 청소 장치에 일반 모드 명령을 전달한다.<br>2. 주행 장치에 시계 방향 90도 제자리 회전 명령을 전달한다.<br>3. 후방 센서 상태를 확인한다.<br>4. 후방 장애물이 감지되지 않으면 현재 진행 방향을 후진으로 Toggle한다.<br>5. 주행 장치에 후진 명령을 전달한다. |
| Alternative/Exception Scenarios | A1. 최초 90도 회전 후 후방 장애물이 감지되면 UC-007을 수행한다.<br>A2. 회전 중 먼지가 새로 감지되면 다음 판단 주기에서 UC-002에 따라 먼지 대응 절차를 우선한다. |
| Result | RVC는 전방 장애물을 피해 후진 방향으로 일반 모드 청소를 계속한다. |

### UC-006. 후진 중 장애물 감지 후 전진 전환

| Item | Description |
| --- | --- |
| Preconditions | 현재 진행 방향은 후진이고, 먼지는 감지되지 않았으며, 후방 장애물이 감지되었다. |
| Main Scenario | 1. SW Controller는 청소 장치에 일반 모드 명령을 전달한다.<br>2. 주행 장치에 반시계 방향 90도 제자리 회전 명령을 전달한다.<br>3. 전방 센서 상태를 확인한다.<br>4. 전방 장애물이 감지되지 않으면 현재 진행 방향을 전진으로 Toggle한다.<br>5. 주행 장치에 전진 명령을 전달한다. |
| Alternative/Exception Scenarios | A1. 최초 90도 회전 후 전방 장애물이 감지되면 UC-007을 수행한다.<br>A2. 회전 중 먼지가 새로 감지되면 다음 판단 주기에서 UC-002에 따라 먼지 대응 절차를 우선한다. |
| Result | RVC는 후방 장애물을 피해 전진 방향으로 일반 모드 청소를 계속한다. |

### UC-007. 90도 단위 회전 중 센서 재확인

| Item | Description |
| --- | --- |
| Preconditions | RVC가 먼지 또는 장애물 대응 절차에서 최초 90도 회전을 완료했고, 대상 센서가 여전히 장애물을 감지한다. |
| Main Scenario | 1. SW Controller는 현재 대응 절차의 회전 방향을 유지한다.<br>2. 주행 장치에 동일 방향 90도 제자리 회전 명령을 전달한다.<br>3. 회전 후 대상 센서를 다시 확인한다.<br>4. 대상 센서가 장애물을 감지하지 않을 때까지 90도 회전과 센서 확인을 반복한다.<br>5. 대상 센서가 장애물을 감지하지 않으면 호출한 유스케이스로 복귀해 진행 방향 Toggle 및 주행 재개를 완료한다. |
| Alternative/Exception Scenarios | A1. 먼지 대응 절차에서 호출된 경우 Boost 모드를 유지한다.<br>A2. 장애물 대응 절차에서 호출된 경우 일반 모드를 유지한다.<br>E1. 대상 센서가 계속 장애물을 감지하는 경우의 최대 회전 횟수 또는 정지 정책은 현재 범위 밖이며 향후 확장 후보로 둔다. |
| Result | RVC는 90도 단위로 회전하며 다음 진행 방향이 열릴 때까지 대상 센서를 재확인한다. |

### UC-008. TCP 명령 인터페이스로 컨트롤러 구동 및 상태 조회

| Item | Description |
| --- | --- |
| Preconditions | `rvc_app`이 TCP endpoint에서 listen 중이고 외부 클라이언트가 접속할 수 있다. |
| Main Scenario | 1. 시뮬레이터/시스템 테스트 클라이언트가 TCP 연결을 생성한다.<br>2. 클라이언트가 line-based command를 전송한다.<br>3. 시스템은 command line 길이와 문법을 검증한다.<br>4. 시스템은 명령을 추상화된 센서 입력, 상태 조회, 초기화, 종료 요청으로 변환한다.<br>5. 센서 입력 명령이면 UC-001~UC-007 중 해당 자동 청소 흐름을 수행한다.<br>6. 상태 조회 명령이면 현재 컨트롤러 상태, 센서 상태, 진행 방향, 최근 출력 명령, 청소 출력 모드를 line-based response로 반환한다.<br>7. 시스템은 처리 결과를 `OK ...` 또는 `ERR ...` 응답으로 반환한다. |
| Alternative/Exception Scenarios | A1. command line이 1024 bytes를 초과하면 `ERR INVALID_ARGUMENT`를 반환하고 연결을 종료한다.<br>A2. 알 수 없는 명령이면 `ERR UNKNOWN_COMMAND`를 반환한다.<br>A3. 인자가 잘못된 명령이면 `ERR INVALID_ARGUMENT`를 반환한다.<br>A4. 현재 상태에서 수행할 수 없는 명령이면 `ERR INVALID_STATE`를 반환한다.<br>A5. `QUIT` 명령이면 `OK BYE`를 반환하고 세션을 종료한다. |
| Result | 외부 클라이언트는 TCP protocol만으로 컨트롤러를 구동하고 상태를 관찰할 수 있으며, Python 시뮬레이터의 지도/렌더링/물리 적용은 C++ 자동 청소 판단과 분리된다. |

## 5. Traceability Matrix

| Requirement ID | Covered By |
| --- | --- |
| FR-001 | UC-001 |
| FR-002 | UC-001 |
| FR-003 | UC-001, UC-005, UC-006, UC-007 |
| FR-004 | UC-006 |
| FR-005 | UC-001, UC-003, UC-004, UC-005, UC-006 |
| FR-006 | UC-002 |
| FR-007 | UC-002 |
| FR-008 | UC-003 |
| FR-009 | UC-003, UC-007 |
| FR-010 | UC-003 |
| FR-011 | UC-004 |
| FR-012 | UC-004, UC-007 |
| FR-013 | UC-004 |
| FR-014 | UC-005 |
| FR-015 | UC-005, UC-007 |
| FR-016 | UC-005 |
| FR-017 | UC-006 |
| FR-018 | UC-006, UC-007 |
| FR-019 | UC-006 |
| FR-020 | UC-003, UC-004, UC-005, UC-006, UC-007 |
| FR-021 | UC-003, UC-004, UC-007 |
| FR-022 | UC-001 |
| FR-023 | UC-008, `docs/command-protocol.md` |
| FR-024 | UC-008, `docs/command-protocol.md` |
| NFR-001 | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006, UC-007 |
| NFR-002 | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006 |
| NFR-003 | UC-002, UC-003, UC-004, UC-005, UC-006, UC-007 |
| NFR-004 | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006, UC-007 |
| NFR-005 | 설계 시 확장 고려사항 |
| NFR-006 | UC-003, UC-004, UC-005, UC-006, UC-007 |
| NFR-007 | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006, UC-007 |
| NFR-008 | UC-008 |
| NFR-009 | UC-001, UC-003, UC-004, UC-005, UC-006, UC-007 |
| NFR-010 | UC-002, UC-003, UC-004, UC-005, UC-006, UC-007, UC-008 |

## 6. Notes

- 본 문서의 use case는 현재 필수 범위인 자동 청소 기능을 기준으로 한다.
- TCP 명령 인터페이스와 Python 시뮬레이터는 UC-008에서 외부 actor와 통합 use case로 표현하고, UC-001~UC-007은 자동 청소 도메인 흐름으로 유지한다.
- 모바일 앱 통신, 머신러닝 기반 판단, 특정 지점 선회 청소는 향후 확장 후보이며 현재 use case에는 포함하지 않는다.
- 90도 단위 회전을 반복해도 대상 센서가 계속 장애물을 감지하는 경우 최대 회전 횟수나 오류/대기 상태를 둘 것인지 결정이 필요하다.
- 전원 ON 직후 초기 진행 방향은 전진으로 가정한다. 다른 초기화 정책이 필요하면 상세 설계에서 별도 요구사항으로 분리한다.
- 기존 좌측/우측 회피 정책과 우측 탐색 프로브 방식은 신규 요구사항의 필수 흐름에서 제외한다.
