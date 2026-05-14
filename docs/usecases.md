# RVC SW Controller Use Cases

## 1. Purpose

이 문서는 `requirements.md`와 `functional-and-non-functional-requirements.md`를 바탕으로 Robot Vacuum Cleaner(RVC) SW Controller의 use case를 정리한다.

현재 범위는 자동 청소 기능이다. 센서, 모터, 브러시의 상세 하드웨어 구현은 제외하고, SW Controller가 입력을 해석해 어떤 동작 명령을 결정해야 하는지에 집중한다.

## 2. Actors

| Actor | Description |
| --- | --- |
| 센서 입력 | 전방, 좌측, 우측 장애물 감지와 먼지 감지 상태를 SW Controller에 제공하는 추상화된 입력이다. |
| 주행 장치 | 전진, 후진, 좌회전, 우회전, 정지 명령을 수행하는 추상화된 출력 대상이다. |
| 청소 장치 | 일반 출력 또는 증가된 출력으로 청소를 수행하는 추상화된 출력 대상이다. |

## 3. Use Case Summary

| ID | Use Case | Primary Actor | Related Requirements |
| --- | --- | --- | --- |
| UC-001 | 장애물이 없을 때 직진 청소 | 센서 입력 | FR-001, FR-003 |
| UC-002 | 전방 장애물 감지 후 정지 | 센서 입력 | FR-004 |
| UC-003 | 좌측 또는 우측 회피 방향 선택 | 센서 입력 | FR-005, FR-008, NFR-006 |
| UC-004 | 방향 전환 후 청소 재개 | 센서 입력 | FR-006, FR-009 |
| UC-005 | 삼면 장애물 감지 후 후진 | 센서 입력 | FR-007 |
| UC-006 | 먼지 감지 이벤트 처리 | 센서 입력 | FR-010, FR-011 |
| UC-007 | 청소 출력 일반 상태 복귀 | 센서 입력 | FR-012 |

## 4. Use Cases

### UC-001. 장애물이 없을 때 직진 청소

| Item | Description |
| --- | --- |
| Preconditions | 자동 청소가 시작되어 있고 전방 장애물이 감지되지 않는다. |
| Main Scenario | 1. 센서 입력에서 전방 장애물 미감지 상태를 확인한다.<br>2. 청소 장치에 일반 청소 출력 명령을 전달한다.<br>3. 주행 장치에 전진 명령을 전달한다.<br>4. 다음 센서 입력 주기까지 전진 청소 상태를 유지한다. |
| Alternative/Exception Scenarios | A1. 전방 장애물이 감지되면 UC-002로 전환한다.<br>A2. 먼지가 감지되면 주행은 유지하고 UC-006으로 전환한다. |
| Result | RVC는 청소하면서 직진한다. |

### UC-002. 전방 장애물 감지 후 정지

| Item | Description |
| --- | --- |
| Preconditions | RVC가 전진 청소 중이다. |
| Main Scenario | 1. 센서 입력에서 전방 장애물 감지 상태를 확인한다.<br>2. SW Controller는 현재 전진 명령을 중단한다.<br>3. 주행 장치에 정지 명령을 전달한다.<br>4. 방향 전환 판단 단계로 상태를 넘긴다. |
| Alternative/Exception Scenarios | A1. 전방, 좌측, 우측 장애물이 모두 감지되면 UC-005로 전환한다.<br>A2. 정지 중 먼지가 감지되어도 장애물 회피 판단을 우선 수행하고, 청소 출력 상태는 UC-006에 따라 별도로 처리한다. |
| Result | RVC는 전방 장애물 앞에서 정지하고 회피 판단을 준비한다. |

### UC-003. 좌측 또는 우측 회피 방향 선택

| Item | Description |
| --- | --- |
| Preconditions | RVC가 장애물 회피를 위해 방향 전환이 필요한 상태이다. |
| Main Scenario | 1. 좌측 및 우측 장애물 감지 상태를 확인한다.<br>2. 한쪽만 이동 가능하면 이동 가능한 방향을 선택한다.<br>3. 양쪽 모두 이동 가능하면 설정된 회피 방향 선택 정책을 적용한다.<br>4. 선택된 방향을 회전 명령 생성 단계로 전달한다. |
| Alternative/Exception Scenarios | A1. 좌측만 장애물이 감지되면 우측을 선택한다.<br>A2. 우측만 장애물이 감지되면 좌측을 선택한다.<br>A3. 좌측과 우측 모두 이동 가능하면 회피 방향 선택 정책에 따라 선택한다.<br>E1. 좌측과 우측 모두 장애물이 감지되면 방향을 선택하지 않고 UC-005로 전환한다. |
| Result | SW Controller는 좌회전 또는 우회전 중 하나의 회피 방향을 결정한다. |

### UC-004. 방향 전환 후 청소 재개

| Item | Description |
| --- | --- |
| Preconditions | RVC가 장애물 회피를 위한 방향 전환을 완료했다. |
| Main Scenario | 1. 전방 이동 가능 여부를 확인한다.<br>2. 이동 가능하면 주행 장치에 전진 명령을 전달한다.<br>3. 청소 장치에 일반 출력 또는 현재 유지 중인 출력 명령을 전달한다.<br>4. 자동 청소 상태로 복귀한다. |
| Alternative/Exception Scenarios | A1. 방향 전환 후 전방 장애물이 다시 감지되면 UC-003을 다시 수행한다.<br>A2. 먼지 대응 상태가 유지 중이면 일반 출력으로 덮어쓰지 않고 증가 출력 상태를 유지한다. |
| Result | RVC는 회피 후 다시 전진하며 청소를 계속한다. |

### UC-005. 삼면 장애물 감지 후 후진

| Item | Description |
| --- | --- |
| Preconditions | 전방, 좌측, 우측 장애물 감지 입력이 모두 활성화되어 있다. |
| Main Scenario | 1. SW Controller는 삼면 장애물 상태를 확인한다.<br>2. 주행 장치에 정지 명령을 전달한다.<br>3. 주행 장치에 후진 명령을 전달한다.<br>4. 후진 동작이 끝난 뒤 방향 전환 판단 단계로 상태를 넘긴다. |
| Alternative/Exception Scenarios | A1. 후진 후 좌측 또는 우측 중 하나가 이동 가능하면 UC-003으로 전환해 회피 방향을 선택한다.<br>E1. 후진 후에도 전방, 좌측, 우측이 모두 막혀 있으면 정지 상태를 유지한다. |
| Result | RVC는 막힌 상태에서 벗어나기 위해 후진한다. |

### UC-006. 먼지 감지 이벤트 처리

| Item | Description |
| --- | --- |
| Preconditions | RVC가 자동 청소 중이며 먼지 감지 입력을 수신할 수 있다. |
| Main Scenario | 1. 센서 입력에서 먼지 감지 상태를 확인한다.<br>2. SW Controller는 먼지 대응 상태로 전환한다.<br>3. 설정된 증가 출력 수준을 조회한다.<br>4. 청소 장치에 증가 출력 명령을 전달한다.<br>5. 증가 출력 유지 타이머를 시작한다. |
| Alternative/Exception Scenarios | A1. 장애물 회피가 동시에 발생하면 주행 상태는 UC-002 또는 UC-005를 따르고 청소 출력 증가 상태는 유지한다. |
| Result | RVC는 먼지 감지 구간에서 강화된 청소 출력을 사용한다. |

### UC-007. 청소 출력 일반 상태 복귀

| Item | Description |
| --- | --- |
| Preconditions | 먼지 감지로 인해 청소 출력이 증가된 상태이다. |
| Main Scenario | 1. 증가 출력 유지 시간이 끝났는지 확인한다.<br>2. 유지 시간이 끝났으면 청소 장치에 일반 출력 명령을 전달한다.<br>3. 먼지 대응 상태를 종료한다.<br>4. 자동 청소 상태를 계속 유지한다. |
| Alternative/Exception Scenarios | A1. 장애물 회피 중 유지 시간이 끝나면 주행 회피 상태는 유지하고 청소 출력만 일반 출력으로 복귀한다. |
| Result | RVC는 일반 청소 출력으로 복귀한다. |

## 5. Traceability Matrix

| Requirement ID | Covered By |
| --- | --- |
| FR-001 | UC-001 |
| FR-003 | UC-001 |
| FR-004 | UC-002 |
| FR-005 | UC-003 |
| FR-006 | UC-004 |
| FR-007 | UC-005 |
| FR-008 | UC-003 |
| FR-009 | UC-004 |
| FR-010 | UC-006 |
| FR-011 | UC-006 |
| FR-012 | UC-007 |
| NFR-001 | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006, UC-007 |
| NFR-002 | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006 |
| NFR-003 | UC-002, UC-003, UC-004, UC-005, UC-006 |
| NFR-004 | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006, UC-007 |
| NFR-005 | 설계 시 확장 고려사항 |
| NFR-006 | UC-003, UC-006 |
| NFR-007 | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006, UC-007 |

## 6. Notes

- 본 문서의 use case는 현재 필수 범위인 자동 청소 기능을 기준으로 한다.
- 모바일 앱 통신, 머신러닝 기반 판단, 특정 지점 선회 청소는 향후 확장 후보이며 현재 use case에는 포함하지 않는다.
- 후진 거리, 회전 각도, 출력 증가 시간, 출력 수준 등 수치값은 추후 설계 단계에서 결정한다.
