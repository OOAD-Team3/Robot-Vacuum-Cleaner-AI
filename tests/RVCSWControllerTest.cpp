#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "rvc/AutomaticCleaning.hpp"
#include "rvc/Commands.hpp"
#include "rvc/RVCSWController.hpp"
#include "rvc/SensorState.hpp"

namespace {

class DrivingDeviceStub : public rvc::DrivingDevice {
public:
    void moveForward() override { record("moveForward"); }
    void moveBackward() override { record("moveBackward"); }
    void turnLeft() override { record("turnLeft"); }
    void turnRight() override { record("turnRight"); }

    void turn(rvc::AvoidanceDirection direction) override {
        record(direction == rvc::AvoidanceDirection::Left ? "turnLeft" : "turnRight");
    }

    void stop() override { record("stop"); }

    std::vector<std::string> calls;
    std::vector<std::string>* orderedCalls{nullptr};

private:
    void record(const std::string& call) {
        calls.push_back(call);
        if (orderedCalls) {
            orderedCalls->push_back(call);
        }
    }
};

class CleaningDeviceStub : public rvc::CleaningDevice {
public:
    void setCleaningPower(rvc::CleaningPowerLevel powerLevel) override {
        record(powerLevel == rvc::CleaningPowerLevel::Normal ? "setNormal" : "setIncreased");
    }

    void keepCleaningPower(rvc::CleaningPowerLevel powerLevel) override {
        record(powerLevel == rvc::CleaningPowerLevel::Normal ? "keepNormal" : "keepIncreased");
    }

    std::vector<std::string> calls;
    std::vector<std::string>* orderedCalls{nullptr};

private:
    void record(const std::string& call) {
        calls.push_back(call);
        if (orderedCalls) {
            orderedCalls->push_back(call);
        }
    }
};

class TimeStub : public rvc::Time {
public:
    void startTimer(rvc::Duration duration) override { startedDurations.push_back(duration.inMilliseconds()); }

    std::vector<int> startedDurations;
};

class RVCSWControllerTest : public testing::Test {
protected:
    void clearDeviceCalls() {
        drive.calls.clear();
        cleaner.calls.clear();
        time.startedDurations.clear();
    }

    DrivingDeviceStub drive;
    CleaningDeviceStub cleaner;
    TimeStub time;
    rvc::RVCSWController controller{drive, cleaner, time};
};

std::vector<rvc::MovementCommandType> movementTypes(const rvc::CommandResult& result) {
    std::vector<rvc::MovementCommandType> types;
    for (const auto& command : result.movementCommands()) {
        types.push_back(command.commandType());
    }
    return types;
}

} // namespace

// 전방 clear 입력 시 normal 청소 전원 설정 후 moveForward가 호출되어 UC-001 기본 주행을 검증한다.
TEST_F(RVCSWControllerTest, UC001MovesForwardAndStartsNormalCleaningWhenFrontIsClear) {
    controller.reportFrontObstacleState(false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_TRUE(time.startedDurations.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

// 전방 clear 입력 시 setNormal이 moveForward보다 먼저 호출되어 문서의 UC-001 호출 순서를 검증한다.
TEST(RVCSWControllerOrderTest, UC001AppliesCleaningCommandBeforeMovementCommand) {
    std::vector<std::string> orderedCalls;
    DrivingDeviceStub drive;
    CleaningDeviceStub cleaner;
    TimeStub time;
    drive.orderedCalls = &orderedCalls;
    cleaner.orderedCalls = &orderedCalls;
    rvc::RVCSWController controller{drive, cleaner, time};

    controller.reportFrontObstacleState(false);

    EXPECT_EQ(orderedCalls, (std::vector<std::string>{"setNormal", "moveForward"}));
}

// 전방 blocked 입력 시 stop만 호출되고 AvoidingObstacle 상태가 되어 UC-002 안전 정지를 검증한다.
TEST_F(RVCSWControllerTest, UC002StopsWhenFrontObstacleIsDetected) {
    controller.reportFrontObstacleState(true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// 복합 센서 snapshot에서 전방 blocked 입력 시 stop과 AvoidingObstacle 상태가 되어 UC-002 경로를 검증한다.
TEST_F(RVCSWControllerTest, UC002CombinedObstacleSnapshotStopsOnFrontObstacle) {
    controller.reportObstacleState(true, false, false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// 좌측 blocked/우측 open 입력 시 turnRight가 호출되어 UC-003 회전 방향을 검증한다.
TEST_F(RVCSWControllerTest, UC003TurnsRightWhenLeftSideIsBlocked) {
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportSideObstacleState(true, false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnRight"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// 우측 blocked/좌측 open 입력 시 turnLeft가 호출되어 UC-003 대칭 회전 방향을 검증한다.
TEST_F(RVCSWControllerTest, UC003TurnsLeftWhenRightSideIsBlocked) {
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportSideObstacleState(false, true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnLeft"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// 양측 open 입력 시 기본 LeftFirst 정책에 따라 turnLeft가 호출되는지 검증한다.
TEST_F(RVCSWControllerTest, UC003TurnsLeftByDefaultWhenBothSidesAreOpen) {
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportSideObstacleState(false, false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnLeft"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// RightFirst 정책에서 양측 open 입력 시 turnRight가 호출되어 정책 주입 효과를 검증한다.
TEST(RVCSWControllerPolicyTest, UC003TurnsRightWhenInjectedPolicyIsRightFirst) {
    DrivingDeviceStub drive;
    CleaningDeviceStub cleaner;
    TimeStub time;
    rvc::CleaningPolicy policy{
        rvc::AvoidanceDirectionPolicy::RightFirst,
        rvc::CleaningPowerLevel::Increased,
        rvc::Duration::seconds(5)};
    rvc::RVCSWController controller{drive, cleaner, time, rvc::AutomaticCleaning{policy}};

    controller.reportFrontObstacleState(true);
    drive.calls.clear();

    controller.reportSideObstacleState(false, false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnRight"});
}

// 양측 blocked/후방 unknown 입력 시 구동 호출 없이 Blocked 상태가 되어 UC-005 대기 흐름을 검증한다.
TEST_F(RVCSWControllerTest, UC003BothSidesBlockedDoesNotMoveBeforeBackStateIsKnown) {
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportSideObstacleState(true, true);

    EXPECT_TRUE(drive.calls.empty());
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

// 일반 주행 상태에서 side sensor만 입력되면 회피 회전이 발생하지 않는지 검증한다.
TEST_F(RVCSWControllerTest, SideObstacleReportDoesNotTurnDuringNormalCleaning) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportSideObstacleState(true, false);

    EXPECT_TRUE(drive.calls.empty());
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

// 회피 상태에서 전방 blocked와 좌우 상태가 함께 들어오면 stop 반복 대신 회피 방향을 재선택하는지 검증한다.
TEST_F(RVCSWControllerTest, AvoidingObstacleSnapshotWithSideStateSelectsAvoidanceDirection) {
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportObstacleState(true, true, false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnRight"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// 회피 상태에서 전방 clear 입력 시 normal 설정 후 moveForward가 호출되어 UC-004 재개 흐름을 검증한다.
TEST_F(RVCSWControllerTest, UC004ResumesForwardCleaningAfterTurnWhenFrontIsClear) {
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportFrontObstacleState(false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

// 회피 상태에서 전체 clear snapshot 입력 시 normal 설정 후 moveForward가 호출되는 복합 UC-004 경로를 검증한다.
TEST_F(RVCSWControllerTest, UC004CombinedObstacleSnapshotResumesAfterAvoidance) {
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportObstacleState(false, false, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

// 전/좌/우 blocked+후방 clear 입력 시 stop 후 moveBackward가 호출되어 UC-005 후진 순서를 검증한다.
TEST_F(RVCSWControllerTest, UC005StopsAndMovesBackwardWhenThreeSidesBlockedAndBackIsClear) {
    controller.reportObstacleState(true, false, true, true);

    EXPECT_EQ(drive.calls, (std::vector<std::string>{"stop", "moveBackward"}));
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

// 사방 blocked 입력 시 stop만 호출되고 Stopped 상태가 되어 UC-005 후진 불가 흐름을 검증한다.
TEST_F(RVCSWControllerTest, UC005StopsWhenThreeSidesAndBackAreBlocked) {
    controller.reportObstacleState(true, true, true, true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Stopped);
}

// 전/좌/우 blocked+후방 unknown 입력 시 구동 호출이 없어 후방 정보 필수 조건을 검증한다.
TEST_F(RVCSWControllerTest, UC005ThreeDirectionSnapshotWaitsForBackSensorBeforeMoving) {
    controller.reportObstacleState(true, true, true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

// 전/좌/우 blocked 이후 후방 clear 입력 시 stop 후 moveBackward가 호출되는 단계적 UC-005 흐름을 검증한다.
TEST_F(RVCSWControllerTest, UC005BackSensorClearCompletesPendingThreeSideObstacleFlow) {
    controller.reportObstacleState(true, true, true);
    clearDeviceCalls();

    controller.reportBackObstacleState(false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveBackward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

// 후진 후 전체 clear snapshot이 들어오면 바로 전진하지 않고 회피 방향 전환을 먼저 수행하는지 검증한다.
TEST_F(RVCSWControllerTest, UC005SelectsAvoidanceDirectionAfterBackwardBeforeResumingForward) {
    controller.reportObstacleState(true, false, true, true);
    clearDeviceCalls();

    controller.reportObstacleState(false, false, false, false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnLeft"});
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// 전/좌/우 blocked 이후 후방 blocked 입력 시 stop만 호출되는 단계적 후진 불가 흐름을 검증한다.
TEST_F(RVCSWControllerTest, UC005BackSensorBlockedCompletesPendingThreeSideObstacleFlowWithStop) {
    controller.reportObstacleState(true, true, true);
    clearDeviceCalls();

    controller.reportBackObstacleState(true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Stopped);
}

// 일반 상태에서 후방 clear만 입력 시 아무 호출도 없어 UC-005 선행조건을 검증한다.
TEST_F(RVCSWControllerTest, BackSensorAloneDoesNotTriggerMovement) {
    controller.reportBackObstacleState(false);

    EXPECT_TRUE(drive.calls.empty());
    EXPECT_TRUE(cleaner.calls.empty());
}

// 먼지 감지 입력 시 setIncreased 후 타이머가 시작되어 UC-006 활성화를 검증한다.
TEST_F(RVCSWControllerTest, UC006IncreasesCleaningPowerAndStartsTimerWhenDustDetected) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setIncreased"});
    ASSERT_EQ(time.startedDurations.size(), 1U);
    EXPECT_GT(time.startedDurations.front(), 0);
}

// custom duration 정책에서 먼지 감지 입력 시 지정한 1234ms 타이머가 전달되는지 검증한다.
TEST(RVCSWControllerPolicyTest, UC006UsesInjectedDustResponseDuration) {
    DrivingDeviceStub drive;
    CleaningDeviceStub cleaner;
    TimeStub time;
    rvc::CleaningPolicy policy{
        rvc::AvoidanceDirectionPolicy::LeftFirst,
        rvc::CleaningPowerLevel::Increased,
        rvc::Duration::milliseconds(1234)};
    rvc::RVCSWController controller{drive, cleaner, time, rvc::AutomaticCleaning{policy}};

    controller.reportFrontObstacleState(false);
    drive.calls.clear();
    cleaner.calls.clear();

    controller.reportDustDetected();

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setIncreased"});
    EXPECT_EQ(time.startedDurations, std::vector<int>{1234});
}

// 먼지 응답 중 전방 blocked 입력 시 stop과 keepIncreased가 호출되어 UC-006 중단 분기를 검증한다.
TEST_F(RVCSWControllerTest, UC006KeepsIncreasedPowerWhenObstacleDetectedDuringDustResponse) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    clearDeviceCalls();

    controller.reportFrontObstacleState(true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"keepIncreased"});
}

// 먼지 응답 중 단계적 UC-005 후방 clear 입력 시 increased를 유지하면서 후진하는지 검증한다.
TEST_F(RVCSWControllerTest, UC006KeepsIncreasedPowerWhenBackSensorCompletesThreeSideFlow) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    controller.reportObstacleState(true, true, true);
    clearDeviceCalls();

    controller.reportBackObstacleState(false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"keepIncreased"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveBackward"});
}

// 먼지 응답 중 전방 clear 입력 시 keepIncreased 후 moveForward가 호출되어 지속 청소 분기를 검증한다.
TEST_F(RVCSWControllerTest, UC006KeepsIncreasedPowerWhileContinuingForwardCleaning) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    clearDeviceCalls();

    controller.reportFrontObstacleState(false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"keepIncreased"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

// 먼지 응답 중 timeout 입력 시 setNormal이 호출되어 UC-007 전원 복귀를 검증한다.
// 회피 중 먼지 감지 입력은 즉시 출력 증가하지 않고 청소 재개 시 UC-006으로 처리되는지 검증한다.
TEST_F(RVCSWControllerTest, UC006DefersDustResponseDuringAvoidanceUntilCleaningResumes) {
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportDustDetected();

    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_TRUE(time.startedDurations.empty());

    controller.reportFrontObstacleState(false);

    EXPECT_EQ(cleaner.calls, (std::vector<std::string>{"setNormal", "setIncreased"}));
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    ASSERT_EQ(time.startedDurations.size(), 1U);
    EXPECT_GT(time.startedDurations.front(), 0);
}

TEST_F(RVCSWControllerTest, UC007ReturnsCleaningPowerToNormalWhenTimerExpires) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    cleaner.calls.clear();

    controller.increasedPowerDurationExpired();

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
}

// 먼지 응답이 없을 때 timeout 입력 시 아무 호출도 없어 비활성 guard를 검증한다.
TEST_F(RVCSWControllerTest, UC007TimeoutWithoutActiveDustResponseDoesNothing) {
    controller.increasedPowerDurationExpired();

    EXPECT_TRUE(drive.calls.empty());
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_TRUE(time.startedDurations.empty());
}

// 먼지+장애물 상태에서 timeout 입력 시 normal만 복귀하고 회피 상태가 유지되는지 검증한다.
TEST_F(RVCSWControllerTest, UC007TimeoutRestoresPowerWithoutChangingAvoidanceStatus) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.increasedPowerDurationExpired();

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_TRUE(drive.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// clear SensorState 입력 시 moveForward와 normal 청소 명령이 생성되는 기본 도메인 결과를 검증한다.
TEST(AutomaticCleaningTest, HandleSensorStateReturnsForwardNormalCleaningForClearPath) {
    rvc::SensorState sensorState;
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleSensorState(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::MoveForward});
    ASSERT_TRUE(result.cleaningCommand().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Normal);
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Cleaning);
}

// 전방 blocked SensorState 입력 시 stop 명령과 AvoidingObstacle 상태가 되는 도메인 정지 판단을 검증한다.
TEST(AutomaticCleaningTest, HandleSensorStateStopsForFrontObstacle) {
    rvc::SensorState sensorState;
    sensorState.updateFrontObstacle(true);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleSensorState(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    EXPECT_FALSE(result.cleaningCommand().has_value());
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// RightFirst 정책과 양측 open 입력 시 Right 방향이 선택되는 정책 기반 회피 결정을 검증한다.
TEST(AutomaticCleaningTest, SelectAvoidanceDirectionUsesRightFirstPolicyWhenBothSidesOpen) {
    rvc::SensorState sensorState;
    sensorState.updateSideObstacles(false, false);
    rvc::CleaningPolicy policy{
        rvc::AvoidanceDirectionPolicy::RightFirst,
        rvc::CleaningPowerLevel::Increased,
        rvc::Duration::seconds(5)};
    rvc::AutomaticCleaning cleaning{policy};

    const auto decision = cleaning.selectAvoidanceDirection(sensorState);

    ASSERT_TRUE(decision.hasSelectedDirection());
    EXPECT_EQ(*decision.selectedDirection(), rvc::AvoidanceDirection::Right);
    EXPECT_FALSE(decision.backwardRequired());
}

// 양측 blocked 입력 시 backwardRequired와 Blocked 상태가 되어 UC-005 위임 결정을 검증한다.
TEST(AutomaticCleaningTest, SelectAvoidanceDirectionRequiresBackwardWhenBothSidesBlocked) {
    rvc::SensorState sensorState;
    sensorState.updateSideObstacles(true, true);
    rvc::AutomaticCleaning cleaning;

    const auto decision = cleaning.selectAvoidanceDirection(sensorState);

    EXPECT_TRUE(decision.backwardRequired());
    EXPECT_FALSE(decision.hasSelectedDirection());
    EXPECT_FALSE(decision.availableDirection());
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Blocked);
}

// 회전 후 전방 blocked 입력 시 다시 stop과 AvoidingObstacle 상태가 되어 재개 실패 분기를 검증한다.
TEST(AutomaticCleaningTest, ResumeAfterTurnStopsAgainWhenFrontStillBlocked) {
    rvc::SensorState sensorState;
    sensorState.updateFrontObstacle(true);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.resumeAfterTurn(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

// 전/좌/우 blocked+후방 clear 입력 시 stop 후 moveBackward 도메인 명령이 생성되는지 검증한다.
TEST(AutomaticCleaningTest, HandleThreeSideObstacleStopsThenMovesBackwardWhenBackIsClear) {
    rvc::SensorState sensorState;
    sensorState.updateObstacles(true, false, true, true);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_EQ(
        movementTypes(result),
        (std::vector<rvc::MovementCommandType>{
            rvc::MovementCommandType::Stop,
            rvc::MovementCommandType::MoveBackward}));
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Blocked);
}

// 이미 Blocked 상태에서 동일 입력 반복 시 stop 중복 없이 moveBackward만 생성되는지 검증한다.
TEST(AutomaticCleaningTest, HandleThreeSideObstacleDoesNotRepeatStopWhenAlreadyBlocked) {
    rvc::SensorState sensorState;
    sensorState.updateObstacles(true, false, true, true);
    rvc::AutomaticCleaning cleaning;
    (void)cleaning.handleThreeSideObstacle(sensorState);

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::MoveBackward});
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Blocked);
}

// 전방 blocked지만 한쪽 open 입력 시 명령이 없어 UC-005 선행조건 guard를 검증한다.
TEST(AutomaticCleaningTest, HandleThreeSideObstacleDoesNothingWhenPreconditionIsNotMet) {
    rvc::SensorState sensorState;
    sensorState.updateObstacles(true, false, true, false);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_TRUE(result.movementCommands().empty());
    EXPECT_FALSE(result.cleaningCommand().has_value());
}

// 사방 blocked 입력 시 stop 명령과 Stopped 상태가 되는 후방 불가 도메인 분기를 검증한다.
TEST(AutomaticCleaningTest, HandleThreeSideObstacleStopsWhenBackIsBlocked) {
    rvc::SensorState sensorState;
    sensorState.updateObstacles(true, true, true, true);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Stopped);
}

// dust=false 입력 시 청소 명령과 타이머가 없어 먼지 감지 guard를 검증한다.
TEST(AutomaticCleaningTest, HandleDustDetectedDoesNothingWhenDustIsFalse) {
    rvc::SensorState sensorState;
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleDustDetected(sensorState);

    EXPECT_FALSE(result.cleaningCommand().has_value());
    EXPECT_FALSE(result.timerDuration().has_value());
}

// dust=true+custom duration 입력 시 increased 명령과 타이머가 생성되는 먼지 응답을 검증한다.
TEST(AutomaticCleaningTest, HandleDustDetectedReturnsIncreasedPowerAndTimer) {
    rvc::SensorState sensorState;
    sensorState.updateDustDetected(true);
    rvc::CleaningPolicy policy{
        rvc::AvoidanceDirectionPolicy::LeftFirst,
        rvc::CleaningPowerLevel::Increased,
        rvc::Duration::milliseconds(77)};
    rvc::AutomaticCleaning cleaning{policy};

    const auto result = cleaning.handleDustDetected(sensorState);

    ASSERT_TRUE(result.cleaningCommand().has_value());
    ASSERT_TRUE(result.timerDuration().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Increased);
    EXPECT_EQ(result.timerDuration()->inMilliseconds(), 77);
    EXPECT_TRUE(cleaning.isDustResponseActive());
}

// 먼지 응답 활성 중 전방 blocked 입력 시 stop과 increased 유지 명령이 함께 생성되는지 검증한다.
TEST(AutomaticCleaningTest, HandleObstacleWhileDustResponseStopsAndKeepsIncreasedPower) {
    rvc::SensorState dustState;
    dustState.updateDustDetected(true);
    rvc::AutomaticCleaning cleaning;
    (void)cleaning.handleDustDetected(dustState);

    rvc::SensorState obstacleState;
    obstacleState.updateFrontObstacle(true);
    const auto result = cleaning.handleObstacleWhileDustResponse(obstacleState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    ASSERT_TRUE(result.cleaningCommand().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Increased);
}

// 먼지 응답 활성 상태에서 timeout 입력 시 normal 명령과 비활성화 처리를 검증한다.
TEST(AutomaticCleaningTest, HandleDustResponseTimeoutExpiresActiveDustResponse) {
    rvc::SensorState dustState;
    dustState.updateDustDetected(true);
    rvc::AutomaticCleaning cleaning;
    (void)cleaning.handleDustDetected(dustState);

    const auto result = cleaning.handleDustResponseTimeout();

    ASSERT_TRUE(result.cleaningCommand().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Normal);
    EXPECT_FALSE(cleaning.isDustResponseActive());
}

// 먼지 응답 clear 후 timeout 입력 시 no-op이 되어 명시적 정리 동작을 검증한다.
TEST(AutomaticCleaningTest, ClearDustResponseStateMakesLaterTimeoutNoOp) {
    rvc::SensorState dustState;
    dustState.updateDustDetected(true);
    rvc::AutomaticCleaning cleaning;
    (void)cleaning.handleDustDetected(dustState);

    cleaning.clearDustResponseState();
    const auto result = cleaning.handleDustResponseTimeout();

    EXPECT_FALSE(cleaning.isDustResponseActive());
    EXPECT_FALSE(result.cleaningCommand().has_value());
}

// pending 표시 후 clear 전까지 먼지 응답이 활성으로 간주되는지 검증한다.
TEST(AutomaticCleaningTest, MarkDustResponsePendingMakesResponseActiveUntilCleared) {
    rvc::AutomaticCleaning cleaning;

    cleaning.markDustResponsePending();

    EXPECT_FALSE(cleaning.isDustResponseActive());
    EXPECT_TRUE(cleaning.isDustResponsePending());
    cleaning.clearDustResponseState();
    EXPECT_FALSE(cleaning.isDustResponseActive());
    EXPECT_FALSE(cleaning.isDustResponsePending());
}

// 전/좌/우 blocked+후방 clear 입력 후 three-side와 후진 가능 조건이 계산되는지 검증한다.
TEST(SensorStateTest, FourDirectionSnapshotMarksThreeSideBlockedAndBackwardAvailable) {
    rvc::SensorState sensorState;

    sensorState.updateObstacles(true, false, true, true);

    EXPECT_TRUE(sensorState.isThreeSideBlocked());
    EXPECT_TRUE(sensorState.canMoveBackward());
    EXPECT_FALSE(sensorState.isBackObstacleDetected());
}

// three-side 상태에서 후방 blocked 갱신 시 후진 가능 여부만 바뀌는지 검증한다.
TEST(SensorStateTest, BackObstacleUpdateChangesBackwardAvailabilityWithoutChangingThreeSideBlock) {
    rvc::SensorState sensorState;
    sensorState.updateObstacles(true, false, true, true);

    sensorState.updateBackObstacle(true);

    EXPECT_TRUE(sensorState.isThreeSideBlocked());
    EXPECT_FALSE(sensorState.canMoveBackward());
    EXPECT_TRUE(sensorState.isBackObstacleDetected());
}

// 좌측 blocked/우측 open 입력 후 파생 side state가 회피 판단에 맞게 계산되는지 검증한다.
TEST(SensorStateTest, SideObstacleStateDerivesAsymmetricSideBlockage) {
    rvc::SensorState sensorState;

    sensorState.updateSideObstacles(true, false);
    const auto sideState = sensorState.sideObstacleState();

    EXPECT_TRUE(sideState.leftBlocked());
    EXPECT_FALSE(sideState.rightBlocked());
    EXPECT_FALSE(sideState.bothBlocked());
    EXPECT_FALSE(sideState.bothOpen());
}

// Left/Right 입력 시 각 Turn 타입과 방향이 보존되어 구동 명령 변환을 검증한다.
TEST(CommandTest, CreateTurnCommandMapsDirectionToTurnCommandType) {
    const auto left = rvc::MovementCommand::createTurnCommand(rvc::AvoidanceDirection::Left);
    const auto right = rvc::MovementCommand::createTurnCommand(rvc::AvoidanceDirection::Right);

    EXPECT_EQ(left.commandType(), rvc::MovementCommandType::TurnLeft);
    ASSERT_TRUE(left.direction().has_value());
    EXPECT_EQ(*left.direction(), rvc::AvoidanceDirection::Left);
    EXPECT_EQ(right.commandType(), rvc::MovementCommandType::TurnRight);
    ASSERT_TRUE(right.direction().has_value());
    EXPECT_EQ(*right.direction(), rvc::AvoidanceDirection::Right);
}

// stop 후 moveBackward 추가 시 명령 순서와 마지막 명령이 보존되는지 검증한다.
TEST(CommandTest, CommandResultPreservesCompoundMovementOrder) {
    auto result = rvc::CommandResult::none()
        .withMovement(rvc::MovementCommand::create(rvc::MovementCommandType::Stop))
        .withMovement(rvc::MovementCommand::create(rvc::MovementCommandType::MoveBackward));

    EXPECT_EQ(
        movementTypes(result),
        (std::vector<rvc::MovementCommandType>{
            rvc::MovementCommandType::Stop,
            rvc::MovementCommandType::MoveBackward}));
    ASSERT_TRUE(result.movementCommand().has_value());
    EXPECT_EQ(result.movementCommand()->commandType(), rvc::MovementCommandType::MoveBackward);
}

// zero duration 입력 시 timer 값이 보존되어 컨트롤러 전달 데이터를 검증한다.
TEST(CommandTest, CommandResultCarriesTimerDuration) {
    auto result = rvc::CommandResult::none().withTimer(rvc::Duration::milliseconds(0));

    ASSERT_TRUE(result.timerDuration().has_value());
    EXPECT_TRUE(result.timerDuration()->isZero());
}

// three-side SensorState 입력 시 backwardRequired와 방향 없음 상태가 되는지 검증한다.
TEST(CommandTest, AvoidanceDecisionMarksBackwardRequiredForThreeSideBlock) {
    rvc::SensorState sensorState;
    sensorState.updateObstacles(true, false, true, true);
    auto decision = rvc::AvoidanceDecision::prepareDirectionDecision();

    decision.evaluateBackwardRequired(sensorState);

    EXPECT_TRUE(decision.backwardRequired());
    EXPECT_FALSE(decision.availableDirection());
    EXPECT_FALSE(decision.hasSelectedDirection());
}

// 방향 선택 후 no-available 처리 시 선택 해제와 backwardRequired=false가 되는지 검증한다.
TEST(CommandTest, AvoidanceDecisionCanRepresentNoAvailableDirectionWithoutBackwardRequirement) {
    auto decision = rvc::AvoidanceDecision::prepareDirectionDecision();
    decision.select(rvc::AvoidanceDirection::Left);

    decision.markNoAvailableDirection();

    EXPECT_FALSE(decision.hasSelectedDirection());
    EXPECT_FALSE(decision.backwardRequired());
    EXPECT_FALSE(decision.availableDirection());
}
