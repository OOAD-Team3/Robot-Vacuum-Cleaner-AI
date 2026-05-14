# RVC SW Controller Use Cases

## 1. Purpose

이 문서는 `requirements.md`와 `functional-and-non-functional-requirements.md`를 바탕으로 Robot Vacuum Cleaner(RVC) SW Controller의 use case와 세부 use case를 정리한다.

현재 범위는 자동 청소 기능이다. 센서, 모터, 브러시, 물걸레 장치의 상세 하드웨어 구현은 제외하고, SW Controller가 입력을 해석해 어떤 동작 명령을 결정해야 하는지에 집중한다.

## 2. Actors

| Actor | Description |
| --- | --- |
| 센서 입력 | 전방, 좌측, 우측 장애물 감지와 먼지 감지 상태를 SW Controller에 제공하는 추상화된 입력이다. |
| 주행 장치 | 전진, 후진, 좌회전, 우회전, 정지 명령을 수행하는 추상화된 출력 대상이다. |
| 청소 장치 | 일반 출력 또는 증가된 출력으로 청소 및 물걸레질을 수행하는 추상화된 출력 대상이다. |

## 3. Use Case Summary

| ID | Use Case | Primary Actor | Related Requirements |
| --- | --- | --- | --- |
| UC-001 | 자동 청소 수행 | 센서 입력 | FR-001, FR-002, FR-003 |
| UC-002 | 전방 장애물 회피 | 센서 입력 | FR-004, FR-005, FR-006 |
| UC-003 | 삼면 장애물 상황 탈출 | 센서 입력 | FR-007, FR-008, FR-009 |
| UC-004 | 먼지 감지 시 청소 출력 증가 | 센서 입력 | FR-010, FR-011, FR-012 |

## 4. High-Level Use Cases

### UC-001. 자동 청소 수행

| Item | Description |
| --- | --- |
| Goal | RVC가 가정용 바닥 표면을 자동으로 청소하고, 필요 시 물걸레질을 함께 수행한다. |
| Primary Actor | 센서 입력 |
| Supporting Actors | 주행 장치, 청소 장치 |
| Preconditions | RVC가 청소 가능한 상태이며, SW Controller가 센서 입력과 장치 출력 인터페이스를 사용할 수 있다. |
| Trigger | 자동 청소 기능이 활성화된다. |
| Main Flow | 1. SW Controller는 자동 청소 상태로 진입한다.<br>2. 청소 장치를 일반 출력으로 활성화한다.<br>3. 물걸레질 기능이 사용 가능한 경우 물걸레질을 함께 활성화한다.<br>4. 전방 장애물이 감지되지 않으면 주행 장치에 전진 명령을 전달한다.<br>5. 청소 중 센서 입력을 지속적으로 확인한다.<br>6. 장애물 또는 먼지 감지 상황이 발생하면 해당 세부 use case를 수행한다. |
| Alternative Flows | A1. 전방 장애물이 감지되면 UC-002를 수행한다.<br>A2. 전방, 좌측, 우측에 모두 장애물이 감지되면 UC-003을 수행한다.<br>A3. 먼지가 감지되면 UC-004를 수행한다. |
| Postconditions | RVC는 청소를 계속 진행하거나, 이동할 수 없는 상태에 따라 정지 상태가 된다. |

### UC-002. 전방 장애물 회피

| Item | Description |
| --- | --- |
| Goal | 전방 장애물을 감지했을 때 충돌 없이 방향을 전환하고 청소를 재개한다. |
| Primary Actor | 센서 입력 |
| Supporting Actors | 주행 장치, 청소 장치 |
| Preconditions | RVC가 자동 청소 중이며 전진 중이다. |
| Trigger | 전방 장애물 감지 입력이 활성화된다. |
| Main Flow | 1. SW Controller는 주행 장치에 정지 명령을 전달한다.<br>2. 회피 방향 선택 정책에 따라 좌측 또는 우측 방향을 선택한다.<br>3. 주행 장치에 선택된 방향으로 회전 명령을 전달한다.<br>4. 회전 후 전방 이동 가능 여부를 확인한다.<br>5. 이동 가능하면 주행 장치에 전진 명령을 전달한다.<br>6. 청소 장치는 청소 동작을 재개하거나 유지한다. |
| Alternative Flows | A1. 회전 후에도 전방 장애물이 감지되면 다시 방향 전환을 시도하거나 UC-003 조건을 평가한다.<br>A2. 좌측과 우측 중 하나가 막혀 있으면 이동 가능한 방향을 우선 선택한다. |
| Postconditions | RVC는 장애물을 피한 방향으로 전진하며 자동 청소를 계속한다. |

### UC-003. 삼면 장애물 상황 탈출

| Item | Description |
| --- | --- |
| Goal | 전방, 좌측, 우측에 모두 장애물이 있는 상황에서 후진 후 방향 전환으로 탈출한다. |
| Primary Actor | 센서 입력 |
| Supporting Actors | 주행 장치, 청소 장치 |
| Preconditions | RVC가 자동 청소 중이며 전방, 좌측, 우측 장애물 감지 입력을 사용할 수 있다. |
| Trigger | 전방, 좌측, 우측 장애물이 모두 감지된다. |
| Main Flow | 1. SW Controller는 주행 장치에 정지 명령을 전달한다.<br>2. 주행 장치에 후진 명령을 전달한다.<br>3. 후진 동작이 끝나면 회피 방향 선택 정책에 따라 좌측 또는 우측 방향을 선택한다.<br>4. 주행 장치에 선택된 방향으로 회전 명령을 전달한다.<br>5. 회전 후 전방 이동 가능 여부를 확인한다.<br>6. 이동 가능하면 전진 명령을 전달하고 청소를 재개한다. |
| Alternative Flows | A1. 후진 후에도 이동 가능 방향이 없으면 정지 상태를 유지한다. |
| Postconditions | RVC는 삼면 장애물 상황에서 벗어나 자동 청소를 계속하거나, 탈출 불가 상태에서 정지한다. |

### UC-004. 먼지 감지 시 청소 출력 증가

| Item | Description |
| --- | --- |
| Goal | 먼지가 감지된 구간에서 일정 시간 동안 청소 출력을 높여 청소 성능을 강화한다. |
| Primary Actor | 센서 입력 |
| Supporting Actors | 청소 장치 |
| Preconditions | RVC가 자동 청소 중이며 먼지 감지 입력을 사용할 수 있다. |
| Trigger | 먼지 감지 입력이 활성화된다. |
| Main Flow | 1. SW Controller는 먼지 감지 이벤트를 수신한다.<br>2. 청소 출력 증가 수준과 유지 시간을 설정값에서 확인한다.<br>3. 청소 장치에 증가된 출력 명령을 전달한다.<br>4. 설정된 시간이 유지되는 동안 증가 출력을 유지한다.<br>5. 시간이 끝나면 청소 장치에 일반 출력 복귀 명령을 전달한다. |
| Alternative Flows | A1. 장애물 회피가 동시에 필요하면 주행 판단은 UC-002 또는 UC-003을 따르고, 청소 출력 상태는 별도로 유지한다. |
| Postconditions | 먼지 대응 시간이 끝난 뒤 RVC는 일반 청소 출력으로 복귀한다. |

## 5. Detailed Use Cases

### DUC-001. 장애물이 없을 때 직진 청소

| Item | Description |
| --- | --- |
| Parent Use Case | UC-001 |
| Related Requirements | FR-001, FR-002, FR-003 |
| Preconditions | 자동 청소가 시작되어 있고 전방 장애물이 감지되지 않는다. |
| Main Scenario | 1. 센서 입력에서 전방 장애물 미감지 상태를 확인한다.<br>2. 청소 장치에 일반 청소 출력 명령을 전달한다.<br>3. 물걸레질이 활성화된 경우 물걸레질 수행 명령을 전달한다.<br>4. 주행 장치에 전진 명령을 전달한다.<br>5. 다음 센서 입력 주기까지 전진 청소 상태를 유지한다. |
| Alternative/Exception Scenarios | A1. 전방 장애물이 감지되면 DUC-002로 전환한다.<br>A2. 먼지가 감지되면 주행은 유지하고 DUC-006으로 전환한다. |
| Result | RVC는 청소하면서 직진한다. |

### DUC-002. 전방 장애물 감지 후 정지

| Item | Description |
| --- | --- |
| Parent Use Case | UC-002 |
| Related Requirements | FR-004 |
| Preconditions | RVC가 전진 청소 중이다. |
| Main Scenario | 1. 센서 입력에서 전방 장애물 감지 상태를 확인한다.<br>2. SW Controller는 현재 전진 명령을 중단한다.<br>3. 주행 장치에 정지 명령을 전달한다.<br>4. 방향 전환 판단 단계로 상태를 넘긴다. |
| Alternative/Exception Scenarios | A1. 전방, 좌측, 우측 장애물이 모두 감지되면 DUC-005로 전환한다.<br>A2. 정지 중 먼지가 감지되어도 장애물 회피 판단을 우선 수행하고, 청소 출력 상태는 DUC-006에 따라 별도로 처리한다. |
| Result | RVC는 전방 장애물 앞에서 정지하고 회피 판단을 준비한다. |

### DUC-003. 좌측 또는 우측 회피 방향 선택

| Item | Description |
| --- | --- |
| Parent Use Case | UC-002, UC-003 |
| Related Requirements | FR-005, FR-008, NFR-006 |
| Preconditions | RVC가 장애물 회피를 위해 방향 전환이 필요한 상태이다. |
| Main Scenario | 1. 좌측 및 우측 장애물 감지 상태를 확인한다.<br>2. 한쪽만 이동 가능하면 이동 가능한 방향을 선택한다.<br>3. 양쪽 모두 이동 가능하면 설정된 회피 방향 선택 정책을 적용한다.<br>4. 선택된 방향을 회전 명령 생성 단계로 전달한다. |
| Alternative/Exception Scenarios | A1. 좌측만 장애물이 감지되면 우측을 선택한다.<br>A2. 우측만 장애물이 감지되면 좌측을 선택한다.<br>A3. 좌측과 우측 모두 이동 가능하면 회피 방향 선택 정책에 따라 선택한다.<br>E1. 좌측과 우측 모두 장애물이 감지되면 방향을 선택하지 않고 DUC-005로 전환한다. |
| Result | SW Controller는 좌회전 또는 우회전 중 하나의 회피 방향을 결정한다. |

### DUC-004. 방향 전환 후 청소 재개

| Item | Description |
| --- | --- |
| Parent Use Case | UC-002, UC-003 |
| Related Requirements | FR-006, FR-009 |
| Preconditions | RVC가 장애물 회피를 위한 방향 전환을 완료했다. |
| Main Scenario | 1. 전방 이동 가능 여부를 확인한다.<br>2. 이동 가능하면 주행 장치에 전진 명령을 전달한다.<br>3. 청소 장치에 일반 출력 또는 현재 유지 중인 출력 명령을 전달한다.<br>4. 자동 청소 상태로 복귀한다. |
| Alternative/Exception Scenarios | A1. 방향 전환 후 전방 장애물이 다시 감지되면 DUC-003을 다시 수행한다.<br>A2. 먼지 대응 상태가 유지 중이면 일반 출력으로 덮어쓰지 않고 증가 출력 상태를 유지한다. |
| Result | RVC는 회피 후 다시 전진하며 청소를 계속한다. |

### DUC-005. 삼면 장애물 감지 후 후진

| Item | Description |
| --- | --- |
| Parent Use Case | UC-003 |
| Related Requirements | FR-007 |
| Preconditions | 전방, 좌측, 우측 장애물 감지 입력이 모두 활성화되어 있다. |
| Main Scenario | 1. SW Controller는 삼면 장애물 상태를 확인한다.<br>2. 주행 장치에 정지 명령을 전달한다.<br>3. 주행 장치에 후진 명령을 전달한다.<br>4. 후진 동작이 끝난 뒤 방향 전환 판단 단계로 상태를 넘긴다. |
| Alternative/Exception Scenarios | A1. 후진 후 좌측 또는 우측 중 하나가 이동 가능하면 DUC-003으로 전환해 회피 방향을 선택한다.<br>E1. 후진 후에도 전방, 좌측, 우측이 모두 막혀 있으면 정지 상태를 유지한다. |
| Result | RVC는 막힌 상태에서 벗어나기 위해 후진한다. |

### DUC-006. 먼지 감지 이벤트 처리

| Item | Description |
| --- | --- |
| Parent Use Case | UC-004 |
| Related Requirements | FR-010, FR-011 |
| Preconditions | RVC가 자동 청소 중이며 먼지 감지 입력을 수신할 수 있다. |
| Main Scenario | 1. 센서 입력에서 먼지 감지 상태를 확인한다.<br>2. SW Controller는 먼지 대응 상태로 전환한다.<br>3. 설정된 증가 출력 수준을 조회한다.<br>4. 청소 장치에 증가 출력 명령을 전달한다.<br>5. 증가 출력 유지 타이머를 시작한다. |
| Alternative/Exception Scenarios | A1. 장애물 회피가 동시에 발생하면 주행 상태는 DUC-002 또는 DUC-005를 따르고 청소 출력 증가 상태는 유지한다. |
| Result | RVC는 먼지 감지 구간에서 강화된 청소 출력을 사용한다. |

### DUC-007. 청소 출력 일반 상태 복귀

| Item | Description |
| --- | --- |
| Parent Use Case | UC-004 |
| Related Requirements | FR-012 |
| Preconditions | 먼지 감지로 인해 청소 출력이 증가된 상태이다. |
| Main Scenario | 1. 증가 출력 유지 시간이 끝났는지 확인한다.<br>2. 유지 시간이 끝났으면 청소 장치에 일반 출력 명령을 전달한다.<br>3. 먼지 대응 상태를 종료한다.<br>4. 자동 청소 상태를 계속 유지한다. |
| Alternative/Exception Scenarios | A1. 장애물 회피 중 유지 시간이 끝나면 주행 회피 상태는 유지하고 청소 출력만 일반 출력으로 복귀한다. |
| Result | RVC는 일반 청소 출력으로 복귀한다. |

## 6. Traceability Matrix

| Requirement ID | Covered By |
| --- | --- |
| FR-001 | UC-001, DUC-001 |
| FR-002 | UC-001, DUC-001 |
| FR-003 | UC-001, DUC-001 |
| FR-004 | UC-002, DUC-002 |
| FR-005 | UC-002, DUC-003 |
| FR-006 | UC-002, DUC-004 |
| FR-007 | UC-003, DUC-005 |
| FR-008 | UC-003, DUC-003 |
| FR-009 | UC-003, DUC-004 |
| FR-010 | UC-004, DUC-006 |
| FR-011 | UC-004, DUC-006 |
| FR-012 | UC-004, DUC-007 |
| NFR-001 | UC-001, UC-002, UC-003, UC-004 |
| NFR-002 | UC-001, UC-002, UC-003, UC-004 |
| NFR-003 | UC-002, UC-003, UC-004 |
| NFR-004 | UC-001, UC-002, UC-003, UC-004 |
| NFR-005 | 설계 시 확장 고려사항 |
| NFR-006 | DUC-003, DUC-006 |
| NFR-007 | UC-001, UC-002, UC-003, UC-004 |

## 7. Notes

- 본 문서의 use case는 현재 필수 범위인 자동 청소 기능을 기준으로 한다.
- 모바일 앱 통신, 머신러닝 기반 판단, 특정 지점 선회 청소는 향후 확장 후보이며 현재 상세 use case에는 포함하지 않는다.
- 후진 거리, 회전 각도, 출력 증가 시간, 출력 수준 등 수치값은 추후 설계 단계에서 결정한다.
