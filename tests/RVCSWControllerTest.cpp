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
    void turnLeft() override { record("turnCounterClockwise90"); }
    void turnRight() override { record("turnClockwise90"); }

    void turn(rvc::AvoidanceDirection direction) override {
        record(direction == rvc::AvoidanceDirection::Left ? "turnCounterClockwise90" : "turnClockwise90");
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
        record(powerLevel == rvc::CleaningPowerLevel::Normal ? "setNormal" : "setBoost");
    }

    void keepCleaningPower(rvc::CleaningPowerLevel powerLevel) override {
        record(powerLevel == rvc::CleaningPowerLevel::Normal ? "keepNormal" : "keepBoost");
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

    void enterBackwardCleaning() {
        controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Blocked, true);
        clearDeviceCalls();

        controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);
        clearDeviceCalls();

        ASSERT_EQ(controller.travelDirection(), rvc::TravelDirection::Backward);
        ASSERT_FALSE(controller.isRotationActive());
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

TEST_F(RVCSWControllerTest, MaintainsForwardNormalCleaningWhenPathIsClear) {
    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_TRUE(time.startedDurations.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_FALSE(controller.isRotationActive());
}

TEST(RVCSWControllerOrderTest, AppliesCleaningCommandBeforeMovementCommand) {
    std::vector<std::string> orderedCalls;
    DrivingDeviceStub drive;
    CleaningDeviceStub cleaner;
    TimeStub time;
    drive.orderedCalls = &orderedCalls;
    cleaner.orderedCalls = &orderedCalls;
    rvc::RVCSWController controller{drive, cleaner, time};

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);

    EXPECT_EQ(orderedCalls, (std::vector<std::string>{"setNormal", "moveForward"}));
}

TEST_F(RVCSWControllerTest, PrioritizesDustOverObstacleWhileMovingForward) {
    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Blocked, true);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setBoost"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnClockwise90"});
    EXPECT_TRUE(time.startedDurations.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Rotating);
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_TRUE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, RepeatsForwardDustRotationUntilBackSensorIsClear) {
    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Blocked, true);
    clearDeviceCalls();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Blocked, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setBoost"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnClockwise90"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Rotating);
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_TRUE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, CompletesForwardDustRotationByTogglingToBackwardNormalCleaning) {
    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Blocked, true);
    clearDeviceCalls();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveBackward"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Cleaning);
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Backward);
    EXPECT_FALSE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, ForwardObstacleStartsClockwiseRotationWithNormalPower) {
    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Unknown, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnClockwise90"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Rotating);
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_TRUE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, ForwardObstacleRotationRepeatsUntilBackSensorIsClear) {
    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Blocked, false);
    clearDeviceCalls();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Blocked, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnClockwise90"});
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_TRUE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, ForwardObstacleRotationCompletesByTogglingToBackward) {
    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Blocked, false);
    clearDeviceCalls();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveBackward"});
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Backward);
    EXPECT_FALSE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, MaintainsBackwardNormalCleaningWhenBackPathIsClear) {
    enterBackwardCleaning();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveBackward"});
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Backward);
    EXPECT_FALSE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, BackwardDustStartsCounterClockwiseRotationWithBoost) {
    enterBackwardCleaning();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Clear, true);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setBoost"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnCounterClockwise90"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Rotating);
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Backward);
    EXPECT_TRUE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, BackwardDustRotationCompletesByTogglingToForward) {
    enterBackwardCleaning();
    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Clear, true);
    clearDeviceCalls();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_FALSE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, BackwardObstacleStartsCounterClockwiseRotationWithNormalPower) {
    enterBackwardCleaning();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Blocked, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnCounterClockwise90"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Rotating);
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Backward);
    EXPECT_TRUE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, BackwardObstacleRotationRepeatsUntilFrontSensorIsClear) {
    enterBackwardCleaning();
    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Blocked, false);
    clearDeviceCalls();

    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Clear, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnCounterClockwise90"});
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Backward);
    EXPECT_TRUE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, BackwardObstacleRotationCompletesByTogglingToForward) {
    enterBackwardCleaning();
    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Blocked, false);
    clearDeviceCalls();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Blocked, false);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setNormal"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"moveForward"});
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_FALSE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, DustPreemptsActiveObstacleRotationOnNextSensorSnapshot) {
    controller.reportSensorSnapshot(true, rvc::BackObstacleInput::Blocked, false);
    clearDeviceCalls();

    controller.reportSensorSnapshot(false, rvc::BackObstacleInput::Blocked, true);

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setBoost"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnClockwise90"});
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Rotating);
    EXPECT_EQ(controller.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_TRUE(controller.isRotationActive());
}

TEST_F(RVCSWControllerTest, SeparateDustCommandStartsDustRotationFromCurrentDirection) {
    controller.reportFrontObstacleState(false);
    clearDeviceCalls();

    controller.reportDustDetected();

    EXPECT_EQ(cleaner.calls, std::vector<std::string>{"setBoost"});
    EXPECT_EQ(drive.calls, std::vector<std::string>{"turnClockwise90"});
    EXPECT_TRUE(time.startedDurations.empty());
    EXPECT_EQ(controller.movementStatus(), rvc::MovementStatus::Rotating);
}

TEST(AutomaticCleaningTest, ClearPathReturnsNormalCommandForCurrentDirection) {
    rvc::SensorState sensorState;
    sensorState.updateSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleSensorState(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::MoveForward});
    ASSERT_TRUE(result.cleaningCommand().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Normal);
    EXPECT_EQ(cleaning.travelDirection(), rvc::TravelDirection::Forward);
    EXPECT_FALSE(cleaning.isRotationActive());
}

TEST(AutomaticCleaningTest, ForwardDustStartsClockwiseRotationTowardBackSensor) {
    rvc::SensorState sensorState;
    sensorState.updateSensorSnapshot(false, rvc::BackObstacleInput::Clear, true);
    rvc::AutomaticCleaning cleaning;

    const auto result = cleaning.handleSensorState(sensorState);

    EXPECT_EQ(
        movementTypes(result),
        std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::TurnClockwise90});
    ASSERT_TRUE(result.cleaningCommand().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Increased);
    EXPECT_TRUE(cleaning.isRotationActive());
    EXPECT_EQ(cleaning.movementStatus(), rvc::MovementStatus::Rotating);
}

TEST(AutomaticCleaningTest, CompletingDustRotationClearsBoostAndTogglesDirection) {
    rvc::SensorState sensorState;
    sensorState.updateSensorSnapshot(false, rvc::BackObstacleInput::Blocked, true);
    rvc::AutomaticCleaning cleaning;
    (void)cleaning.handleSensorState(sensorState);

    sensorState.updateSensorSnapshot(false, rvc::BackObstacleInput::Clear, false);
    const auto result = cleaning.handleSensorState(sensorState);

    EXPECT_EQ(movementTypes(result), std::vector<rvc::MovementCommandType>{rvc::MovementCommandType::MoveBackward});
    ASSERT_TRUE(result.cleaningCommand().has_value());
    EXPECT_EQ(result.cleaningCommand()->powerLevel(), rvc::CleaningPowerLevel::Normal);
    EXPECT_EQ(cleaning.travelDirection(), rvc::TravelDirection::Backward);
    EXPECT_FALSE(cleaning.isRotationActive());
}

TEST(SensorStateTest, SnapshotCarriesFrontBackAndDustTogether) {
    rvc::SensorState sensorState;

    sensorState.updateSensorSnapshot(true, rvc::BackObstacleInput::Blocked, true);

    EXPECT_TRUE(sensorState.isFrontObstacleDetected());
    EXPECT_TRUE(sensorState.isBackObstacleStateKnown());
    EXPECT_TRUE(sensorState.isBackObstacleDetected());
    EXPECT_TRUE(sensorState.isDustDetected());
    EXPECT_TRUE(sensorState.obstacleDetectedIn(rvc::TravelDirection::Forward));
    EXPECT_TRUE(sensorState.obstacleDetectedIn(rvc::TravelDirection::Backward));
}

TEST(SensorStateTest, UnknownBackSensorIsNotClearForBackwardTarget) {
    rvc::SensorState sensorState;

    sensorState.updateSensorSnapshot(false, rvc::BackObstacleInput::Unknown, false);

    EXPECT_FALSE(sensorState.isBackObstacleStateKnown());
    EXPECT_FALSE(sensorState.targetSensorIsClear(rvc::TargetSensor::Back));
}

TEST(CommandTest, ExplicitTurnCommandsRepresentNinetyDegreeRotation) {
    const auto clockwise = rvc::MovementCommand::create(rvc::MovementCommandType::TurnClockwise90);
    const auto counterClockwise = rvc::MovementCommand::create(rvc::MovementCommandType::TurnCounterClockwise90);

    EXPECT_EQ(clockwise.commandType(), rvc::MovementCommandType::TurnClockwise90);
    EXPECT_EQ(counterClockwise.commandType(), rvc::MovementCommandType::TurnCounterClockwise90);
}
