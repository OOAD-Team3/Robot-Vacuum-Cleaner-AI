# Robot Vacuum Cleaner AI

Robot Vacuum Cleaner(RVC) SW Controller의 요구사항과 설계 문서, 코드를 관리하는 저장소입니다. 현재 범위는 자동 청소 기능이며, 센서/모터/청소 장치의 상세 하드웨어 구현은 추상화된 입력과 출력으로 다룹니다.

### Folder Structure

```text
.
├── README.md
└── docs
    ├── requirements.md                              # 전체 범위, 요구사항, 제외 범위, 확장 후보
    ├── functional-and-non-functional-requirements.md # FR/NFR ID 기준 문서
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
