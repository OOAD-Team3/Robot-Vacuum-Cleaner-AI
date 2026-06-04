#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "rvc/AutomaticCleaning.hpp"
#include "rvc/Commands.hpp"
#include "rvc/DustResponse.hpp"
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

    void enterAvoidance() {
        controller.reportFrontObstacleState(true);
        clearDeviceCalls();
    }

    void startRightProbe() {
        enterAvoidance();
        controller.reportLeftObstacleState(true);
        clearDeviceCalls();
    }

    void confirmThreeSideBlocked() {
        startRightProbe();
        controller.reportFrontObstacleState(true);
        clearDeviceCalls();
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

void confirmThreeSideBlocked(rvc::AutomaticCleaning& cleaning, rvc::SensorState& sensorState) {
    sensorState.updateFrontObstacle(true);
    sensorState.updateLeftObstacle(true);

    const auto probeDecision = cleaning.selectAvoidanceDirection(sensorState);
    ASSERT_TRUE(probeDecision.rightProbeRequired());

    sensorState.updateFrontObstacle(true);
    const auto blockedDecision = cleaning.selectAvoidanceDirection(sensorState);
    ASSERT_TRUE(blockedDecision.backwardRequired());
}

} // namespace

TEST_F(RVCSWControllerTest, UC001MovesForwardAndStartsNormalCleaningWhenFrontIsClear) {
    controller.reportFrontObstacleState(false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_TRUE(time.startedDurations.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

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

TEST_F(RVCSWControllerTest, UC002StopsWhenFrontObstacleIsDetected) {
    controller.reportFrontObstacleState(true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, UC002CombinedObstacleSnapshotStopsOnFrontObstacle) {
    controller.reportObstacleState(true, rvc::BackObstacleInput::Unknown, false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, LeftObstacleReportDoesNotTurnDuringNormalCleaning) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportLeftObstacleState(true);

    EXPECT_TRUE(drive.calls.empty());
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST_F(RVCSWControllerTest, UC003TurnsLeftWhenLeftSideIsOpen) {
    enterAvoidance();

    controller.reportLeftObstacleState(false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnLeft"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, UC003StartsRightProbeWhenLeftSideIsBlocked) {
    enterAvoidance();

    controller.reportLeftObstacleState(true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnRight"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, UC003LeftOnlyReportDoesNotResolveActiveRightProbe) {
    startRightProbe();

    controller.reportLeftObstacleState(true);

    EXPECT_TRUE(drive.calls.empty());
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, UC003FrontLeftSnapshotStartsRightProbeDuringAvoidance) {
    enterAvoidance();

    controller.reportObstacleState(true, rvc::BackObstacleInput::Unknown, true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnRight"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, UC003RightProbeOpenResumesWithoutReturningToOriginalHeading) {
    startRightProbe();

    controller.reportFrontObstacleState(false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST_F(RVCSWControllerTest, UC003RightProbeOpenSnapshotResumesWithoutReturningToOriginalHeading) {
    startRightProbe();

    controller.reportObstacleState(false, rvc::BackObstacleInput::Unknown, true);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST_F(RVCSWControllerTest, UC003RightProbeBlockedReturnsToOriginalHeadingAndWaitsForBackSensor) {
    startRightProbe();

    controller.reportFrontObstacleState(true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnLeft"});
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST_F(RVCSWControllerTest, UC005StopsWhenBackStateIsUnknownAfterRightProbeBlocked) {
    confirmThreeSideBlocked();

    controller.reportBackObstacleState(rvc::BackObstacleInput::Unknown);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST_F(RVCSWControllerTest, UC005CombinedSnapshotWithUnknownBackStopsAfterRightProbeBlocked) {
    confirmThreeSideBlocked();

    controller.reportObstacleState(true, rvc::BackObstacleInput::Unknown, true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST_F(RVCSWControllerTest, UC005CombinedSnapshotWithClearBackMovesBackwardAfterRightProbeBlocked) {
    confirmThreeSideBlocked();

    controller.reportObstacleState(true, rvc::BackObstacleInput::Clear, true);

    EXPECT_EQ(drive.calls, (std::vector<std::string>{"stop", "moveBackward"}));
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST_F(RVCSWControllerTest, UC005CombinedSnapshotWithBlockedBackStopsAfterRightProbeBlocked) {
    confirmThreeSideBlocked();

    controller.reportObstacleState(true, rvc::BackObstacleInput::Blocked, true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Stopped);
}

TEST_F(RVCSWControllerTest, UC005StopsAndMovesBackwardWhenBackIsClearAfterRightProbeBlocked) {
    confirmThreeSideBlocked();

    controller.reportBackObstacleState(rvc::BackObstacleInput::Clear);

    EXPECT_EQ(drive.calls, (std::vector<std::string>{"stop", "moveBackward"}));
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST_F(RVCSWControllerTest, UC005BackSensorClearAfterUnknownStopMovesBackwardWithoutRepeatingStop) {
    confirmThreeSideBlocked();
    controller.reportBackObstacleState(rvc::BackObstacleInput::Unknown);
    clearDeviceCalls();

    controller.reportBackObstacleState(rvc::BackObstacleInput::Clear);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveBackward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST_F(RVCSWControllerTest, UC005StopsWhenBackIsBlockedAfterRightProbeBlocked) {
    confirmThreeSideBlocked();

    controller.reportBackObstacleState(rvc::BackObstacleInput::Blocked);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Stopped);
}

TEST_F(RVCSWControllerTest, UC005AfterBackwardUsesLeftSensorBeforeResuming) {
    confirmThreeSideBlocked();
    controller.reportBackObstacleState(rvc::BackObstacleInput::Clear);
    clearDeviceCalls();

    controller.reportLeftObstacleState(false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnLeft"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, UC005AfterBackwardCanStartAnotherRightProbeWhenLeftIsStillBlocked) {
    confirmThreeSideBlocked();
    controller.reportBackObstacleState(rvc::BackObstacleInput::Clear);
    clearDeviceCalls();

    controller.reportLeftObstacleState(true);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnRight"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, UC005CombinedSnapshotAfterBackwardUsesLeftSensorBeforeResuming) {
    confirmThreeSideBlocked();
    controller.reportBackObstacleState(rvc::BackObstacleInput::Clear);
    clearDeviceCalls();

    controller.reportObstacleState(true, rvc::BackObstacleInput::Unknown, false);

    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnLeft"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST_F(RVCSWControllerTest, BackSensorAloneDoesNotTriggerMovement) {
    controller.reportBackObstacleState(rvc::BackObstacleInput::Clear);

    EXPECT_TRUE(drive.calls.empty());
    EXPECT_TRUE(cleaner.calls.empty());
}

TEST_F(RVCSWControllerTest, UC004ResumesForwardCleaningAfterTurnWhenFrontIsClear) {
    enterAvoidance();

    controller.reportFrontObstacleState(false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST_F(RVCSWControllerTest, UC004CombinedObstacleSnapshotResumesAfterAvoidance) {
    enterAvoidance();

    controller.reportObstacleState(false, rvc::BackObstacleInput::Unknown, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST_F(RVCSWControllerTest, UC006IncreasesCleaningPowerAndStartsTimerWhenDustDetected) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setIncreased"});
    ASSERT_EQ(time.startedDurations.size(), 1U);
    EXPECT_GT(time.startedDurations.front(), 0);
}

TEST(RVCSWControllerPolicyTest, UC006UsesInjectedDustResponseDuration) {
    DrivingDeviceStub drive;
    CleaningDeviceStub cleaner;
    TimeStub time;
    rvc::CleaningPolicy policy{rvc::CleaningPowerLevel::Increased, rvc::Duration::milliseconds(1234)};
    rvc::RVCSWController controller{drive, cleaner, time, rvc::AutomaticCleaning{policy}};

    controller.reportFrontObstacleState(false);
    drive.calls.clear();
    cleaner.calls.clear();

    controller.reportDustDetected();

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setIncreased"});
    EXPECT_EQ(time.startedDurations, std::vector<int>{1234});
}

TEST(RVCSWControllerPolicyTest, UC006ZeroDustResponseDurationDoesNotStartTimer) {
    DrivingDeviceStub drive;
    CleaningDeviceStub cleaner;
    TimeStub time;
    rvc::CleaningPolicy policy{rvc::CleaningPowerLevel::Increased, rvc::Duration::milliseconds(0)};
    rvc::RVCSWController controller{drive, cleaner, time, rvc::AutomaticCleaning{policy}};

    controller.reportFrontObstacleState(false);
    drive.calls.clear();
    cleaner.calls.clear();

    controller.reportDustDetected();

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setIncreased"});
    EXPECT_TRUE(time.startedDurations.empty());
}

TEST_F(RVCSWControllerTest, UC006KeepsIncreasedPowerWhenObstacleDetectedDuringDustResponse) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    clearDeviceCalls();

    controller.reportFrontObstacleState(true);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"keepIncreased"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
}

TEST_F(RVCSWControllerTest, UC006CombinedSnapshotKeepsIncreasedPowerWhenObstacleDetectedDuringDustResponse) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    clearDeviceCalls();

    controller.reportObstacleState(true, rvc::BackObstacleInput::Unknown, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"keepIncreased"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"stop"});
}

TEST_F(RVCSWControllerTest, UC006CombinedSnapshotKeepsIncreasedPowerWhileContinuingForwardCleaning) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    clearDeviceCalls();

    controller.reportObstacleState(false, rvc::BackObstacleInput::Clear, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"keepIncreased"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST_F(RVCSWControllerTest, UC006KeepsIncreasedPowerWhenRightProbeOpenResumesCleaning) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    controller.reportFrontObstacleState(true);
    controller.reportLeftObstacleState(true);
    clearDeviceCalls();

    controller.reportFrontObstacleState(false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"keepIncreased"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST_F(RVCSWControllerTest, UC006KeepsIncreasedPowerWhenBackSensorCompletesThreeSideFlow) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();
    controller.reportFrontObstacleState(true);
    controller.reportLeftObstacleState(true);
    controller.reportFrontObstacleState(true);
    clearDeviceCalls();

    controller.reportBackObstacleState(rvc::BackObstacleInput::Clear);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"keepIncreased"});
    EXPECT_EQ(drive.calls, (std::vector<std::string>{"stop", "moveBackward"}));
}

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

TEST_F(RVCSWControllerTest, UC007TimeoutWithoutActiveDustResponseDoesNothing) {
    controller.increasedPowerDurationExpired();

    EXPECT_TRUE(drive.calls.empty());
    EXPECT_TRUE(cleaner.calls.empty());
    EXPECT_TRUE(time.startedDurations.empty());
}

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

TEST(AutomaticCleaningTest, RightDirectionProbeTracksBlockedAndClearStates) {
    rvc::RightDirectionProbe probe;

    probe.start();
    EXPECT_TRUE(probe.isActive());
    EXPECT_EQ(probe.result(), rvc::RightProbeResult::Unknown);
    EXPECT_FALSE(probe.restoreOriginalHeadingRequired());

    probe.resolveWithFrontObstacle(true);
    EXPECT_FALSE(probe.isActive());
    EXPECT_TRUE(probe.isBlocked());
    EXPECT_FALSE(probe.isOpen());
    EXPECT_TRUE(probe.restoreOriginalHeadingRequired());
    EXPECT_EQ(probe.result(), rvc::RightProbeResult::Blocked);

    probe.clear();
    EXPECT_FALSE(probe.isActive());
    EXPECT_FALSE(probe.isBlocked());
    EXPECT_FALSE(probe.isOpen());
    EXPECT_FALSE(probe.restoreOriginalHeadingRequired());
    EXPECT_EQ(probe.result(), rvc::RightProbeResult::Unknown);
}

TEST(AutomaticCleaningTest, RightDirectionProbeTracksOpenState) {
    rvc::RightDirectionProbe probe;

    probe.start();
    probe.resolveWithFrontObstacle(false);

    EXPECT_FALSE(probe.isActive());
    EXPECT_TRUE(probe.isOpen());
    EXPECT_FALSE(probe.isBlocked());
    EXPECT_FALSE(probe.restoreOriginalHeadingRequired());
    EXPECT_EQ(probe.result(), rvc::RightProbeResult::Open);
}

TEST(AutomaticCleaningTest, HandleSensorStateReturnsForwardNormalCleaningForClearPath) {
    rvc::SensorState sensorState;
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleSensorState(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::MoveForward});
    ASSERT_TRUE(result.cleaningCommand().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Normal);
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST(AutomaticCleaningTest, HandleSensorStateStopsForFrontObstacle) {
    rvc::SensorState sensorState;
    sensorState.updateFrontObstacle(true);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleSensorState(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    EXPECT_FALSE(result.cleaningCommand().has_value());
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST(AutomaticCleaningTest, SelectAvoidanceDirectionPrioritizesOpenLeftSide) {
    rvc::SensorState sensorState;
    sensorState.updateLeftObstacle(false);
    rvc::AutomaticCleaning cleaning;

    const auto decision = cleaning.selectAvoidanceDirection(sensorState);

    ASSERT_TRUE(decision.hasSelectedDirection());
    EXPECT_EQ(*decision.selectedDirection(), rvc::AvoidanceDirection::Left);
    EXPECT_FALSE(decision.rightProbeRequired());
    EXPECT_FALSE(decision.backwardRequired());
}

TEST(AutomaticCleaningTest, SelectAvoidanceDirectionRequiresRightProbeWhenLeftSideIsBlocked) {
    rvc::SensorState sensorState;
    sensorState.updateLeftObstacle(true);
    rvc::AutomaticCleaning cleaning;

    const auto decision = cleaning.selectAvoidanceDirection(sensorState);

    EXPECT_TRUE(decision.rightProbeRequired());
    EXPECT_FALSE(decision.hasSelectedDirection());
    EXPECT_FALSE(decision.backwardRequired());
    EXPECT_TRUE(cleaning.isRightProbeActive());
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST(AutomaticCleaningTest, RightProbeOpenSelectsRightDirection) {
    rvc::SensorState sensorState;
    sensorState.updateLeftObstacle(true);
    rvc::AutomaticCleaning cleaning;
    (void)cleaning.selectAvoidanceDirection(sensorState);

    sensorState.updateFrontObstacle(false);
    const auto decision = cleaning.selectAvoidanceDirection(sensorState);

    ASSERT_TRUE(decision.hasSelectedDirection());
    EXPECT_EQ(*decision.selectedDirection(), rvc::AvoidanceDirection::Right);
    EXPECT_FALSE(decision.backwardRequired());
    EXPECT_FALSE(cleaning.isRightProbeActive());
}

TEST(AutomaticCleaningTest, RightProbeBlockedMarksBackwardRequired) {
    rvc::SensorState sensorState;
    sensorState.updateLeftObstacle(true);
    rvc::AutomaticCleaning cleaning;
    (void)cleaning.selectAvoidanceDirection(sensorState);

    sensorState.updateFrontObstacle(true);
    const auto decision = cleaning.selectAvoidanceDirection(sensorState);

    EXPECT_TRUE(decision.backwardRequired());
    EXPECT_FALSE(decision.hasSelectedDirection());
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST(AutomaticCleaningTest, HandleThreeSideObstacleStopsWhenBackStateIsUnknown) {
    rvc::SensorState sensorState;
    rvc::AutomaticCleaning cleaning;
    confirmThreeSideBlocked(cleaning, sensorState);

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST(AutomaticCleaningTest, HandleThreeSideObstacleStopsThenMovesBackwardWhenBackIsClear) {
    rvc::SensorState sensorState;
    rvc::AutomaticCleaning cleaning;
    confirmThreeSideBlocked(cleaning, sensorState);
    sensorState.updateBackObstacle(false);

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_EQ(
        movementTypes(result),
        (std::vector<rvc::MovementCommandType>{
            rvc::MovementCommandType::Stop,
            rvc::MovementCommandType::MoveBackward}));
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST(AutomaticCleaningTest, HandleThreeSideObstacleDoesNotRepeatStopAfterUnknownBackStateStopped) {
    rvc::SensorState sensorState;
    rvc::AutomaticCleaning cleaning;
    confirmThreeSideBlocked(cleaning, sensorState);
    (void)cleaning.handleThreeSideObstacle(sensorState);
    sensorState.updateBackObstacle(false);

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::MoveBackward});
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Blocked);
}

TEST(AutomaticCleaningTest, HandleThreeSideObstacleStopsWhenBackIsBlocked) {
    rvc::SensorState sensorState;
    rvc::AutomaticCleaning cleaning;
    confirmThreeSideBlocked(cleaning, sensorState);
    sensorState.updateBackObstacle(true);

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Stopped);
}

TEST(AutomaticCleaningTest, HandleThreeSideObstacleDoesNothingWhenRightDirectionIsNotConfirmed) {
    rvc::SensorState sensorState;
    sensorState.updateObstacles(true, false, true);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleThreeSideObstacle(sensorState);

    EXPECT_TRUE(result.movementCommands().empty());
    EXPECT_FALSE(result.cleaningCommand().has_value());
}

TEST(AutomaticCleaningTest, ResumeAfterTurnStopsAgainWhenFrontStillBlocked) {
    rvc::SensorState sensorState;
    sensorState.updateFrontObstacle(true);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.resumeAfterTurn(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::AvoidingObstacle);
}

TEST(AutomaticCleaningTest, HandleDustDetectedDoesNothingWhenDustIsFalse) {
    rvc::SensorState sensorState;
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleDustDetected(sensorState);

    EXPECT_FALSE(result.cleaningCommand().has_value());
    EXPECT_FALSE(result.timerDuration().has_value());
}

TEST(AutomaticCleaningTest, HandleDustDetectedReturnsIncreasedPowerAndTimer) {
    rvc::SensorState sensorState;
    sensorState.updateDustDetected(true);
    rvc::CleaningPolicy policy{rvc::CleaningPowerLevel::Increased, rvc::Duration::milliseconds(77)};
    rvc::AutomaticCleaning cleaning{policy};

    const auto result = cleaning.handleDustDetected(sensorState);

    ASSERT_TRUE(result.cleaningCommand().has_value());
    ASSERT_TRUE(result.timerDuration().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Increased);
    EXPECT_EQ(result.timerDuration()->inMilliseconds(), 77);
    EXPECT_TRUE(cleaning.isDustResponseActive());
}

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

TEST(AutomaticCleaningTest, HandleObstacleWhileDustResponseWithoutActiveDustDoesNotAddCleaningPower) {
    rvc::AutomaticCleaning cleaning;
    rvc::SensorState obstacleState;
    obstacleState.updateFrontObstacle(true);

    const auto result = cleaning.handleObstacleWhileDustResponse(obstacleState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::Stop});
    EXPECT_FALSE(result.cleaningCommand().has_value());
}

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

TEST(AutomaticCleaningTest, MarkDustResponsePendingMakesResponsePendingUntilCleared) {
    rvc::AutomaticCleaning cleaning;

    cleaning.markDustResponsePending();

    EXPECT_FALSE(cleaning.isDustResponseActive());
    EXPECT_TRUE(cleaning.isDustResponsePending());
    cleaning.clearDustResponseState();
    EXPECT_FALSE(cleaning.isDustResponseActive());
    EXPECT_FALSE(cleaning.isDustResponsePending());
}

TEST(AutomaticCleaningTest, KeepMovementStatusDoesNotChangeCurrentStatus) {
    rvc::AutomaticCleaning cleaning;

    cleaning.changeMovementStatus(rvc::MovementStatus::Cleaning);
    cleaning.keepMovementStatus(rvc::MovementStatus::Blocked);
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Cleaning);

    cleaning.keepMovementStatus(rvc::MovementStatus::Cleaning);
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Cleaning);
}

TEST(DustResponseTest, StartAndExpireExposeCurrentState) {
    rvc::DustResponse response;

    response.start(rvc::CleaningPowerLevel::Increased, rvc::Duration::milliseconds(42));

    EXPECT_TRUE(response.isActive());
    EXPECT_EQ(response.currentPowerLevel(), rvc::CleaningPowerLevel::Increased);
    EXPECT_EQ(response.duration().inMilliseconds(), 42);

    response.expire();

    EXPECT_FALSE(response.isActive());
    EXPECT_EQ(response.currentPowerLevel(), rvc::CleaningPowerLevel::Normal);
    EXPECT_TRUE(response.duration().isZero());
}

TEST(SensorStateTest, FrontLeftSnapshotKeepsBackStateUnknown) {
    rvc::SensorState sensorState;

    sensorState.updateObstacles(true, true);

    EXPECT_TRUE(sensorState.isFrontObstacleDetected());
    EXPECT_TRUE(sensorState.isLeftObstacleDetected());
    EXPECT_FALSE(sensorState.isBackObstacleStateKnown());
    EXPECT_FALSE(sensorState.canMoveBackward());
}

TEST(SensorStateTest, BackObstacleUpdateChangesBackwardAvailability) {
    rvc::SensorState sensorState;

    sensorState.updateBackObstacle(false);
    EXPECT_TRUE(sensorState.isBackObstacleStateKnown());
    EXPECT_FALSE(sensorState.isBackObstacleDetected());
    EXPECT_TRUE(sensorState.canMoveBackward());

    sensorState.updateBackObstacle(true);
    EXPECT_TRUE(sensorState.isBackObstacleDetected());
    EXPECT_FALSE(sensorState.canMoveBackward());
}

TEST(SensorStateTest, ObstacleSnapshotCarriesKnownBackState) {
    rvc::SensorState sensorState;

    sensorState.updateObstacles(true, false, true);

    EXPECT_TRUE(sensorState.isFrontObstacleDetected());
    EXPECT_TRUE(sensorState.isLeftObstacleDetected());
    EXPECT_TRUE(sensorState.isBackObstacleStateKnown());
    EXPECT_FALSE(sensorState.isBackObstacleDetected());
    EXPECT_TRUE(sensorState.canMoveBackward());
}

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

TEST(CommandTest, CommandResultCarriesTimerDuration) {
    auto result = rvc::CommandResult::none().withTimer(rvc::Duration::milliseconds(0));

    ASSERT_TRUE(result.timerDuration().has_value());
    EXPECT_TRUE(result.timerDuration()->isZero());
}

TEST(CommandTest, AvoidanceDecisionCanRepresentRightProbeRequirement) {
    auto decision = rvc::AvoidanceDecision::prepareDirectionDecision();

    decision.markRightProbeRequired();

    EXPECT_TRUE(decision.rightProbeRequired());
    EXPECT_FALSE(decision.hasSelectedDirection());
    EXPECT_FALSE(decision.backwardRequired());
    EXPECT_TRUE(decision.availableDirection());
}

TEST(CommandTest, AvoidanceDecisionCanRepresentNoAvailableDirectionWithoutBackwardRequirement) {
    auto decision = rvc::AvoidanceDecision::prepareDirectionDecision();
    decision.select(rvc::AvoidanceDirection::Left);

    decision.markNoAvailableDirection();

    EXPECT_FALSE(decision.hasSelectedDirection());
    EXPECT_FALSE(decision.backwardRequired());
    EXPECT_FALSE(decision.availableDirection());
}
