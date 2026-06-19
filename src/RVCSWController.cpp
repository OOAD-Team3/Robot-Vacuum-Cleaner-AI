#include "rvc/RVCSWController.hpp"

namespace rvc {

RVCSWController::RVCSWController(DrivingDevice& drivingDevice, CleaningDevice& cleaningDevice, Time& time)
    : drivingDevice_(drivingDevice), cleaningDevice_(cleaningDevice), time_(time) {}

RVCSWController::RVCSWController(
    DrivingDevice& drivingDevice,
    CleaningDevice& cleaningDevice,
    Time& time,
    AutomaticCleaning automaticCleaning)
    : drivingDevice_(drivingDevice),
      cleaningDevice_(cleaningDevice),
      time_(time),
      automaticCleaning_(automaticCleaning) {}

void RVCSWController::reportFrontObstacleState(bool frontObstacleDetected) {
    sensorState_.updateFrontObstacle(frontObstacleDetected);
    evaluateCurrentState();
}

void RVCSWController::reportBackObstacleState(BackObstacleInput backObstacleDetected) {
    if (backObstacleDetected == BackObstacleInput::Unknown) {
        sensorState_.clearBackObstacleState();
    } else {
        sensorState_.updateBackObstacle(backObstacleDetected == BackObstacleInput::Blocked);
    }

    evaluateCurrentState();
}

void RVCSWController::reportLeftObstacleState(bool leftObstacleDetected) {
    sensorState_.updateLeftObstacle(leftObstacleDetected);
}

void RVCSWController::reportObstacleState(
    bool frontObstacleDetected,
    BackObstacleInput backObstacleDetected,
    bool leftObstacleDetected) {
    sensorState_.updateLeftObstacle(leftObstacleDetected);
    sensorState_.updateSensorSnapshot(frontObstacleDetected, backObstacleDetected, sensorState_.isDustDetected());
    evaluateCurrentState();
}

void RVCSWController::reportSensorSnapshot(
    bool frontObstacleDetected,
    BackObstacleInput backObstacleDetected,
    bool dustDetected) {
    sensorState_.updateSensorSnapshot(frontObstacleDetected, backObstacleDetected, dustDetected);
    evaluateCurrentState();
}

void RVCSWController::reportDustDetected() {
    sensorState_.updateDustDetected(true);
    evaluateCurrentState();
}

void RVCSWController::increasedPowerDurationExpired() {
    apply(automaticCleaning_.handleDustResponseTimeout());
}

MovementStatus RVCSWController::movementStatus() const {
    return automaticCleaning_.movementStatus();
}

TravelDirection RVCSWController::travelDirection() const {
    return automaticCleaning_.travelDirection();
}

bool RVCSWController::isRotationActive() const {
    return automaticCleaning_.isRotationActive();
}

void RVCSWController::evaluateCurrentState() {
    apply(automaticCleaning_.handleSensorState(sensorState_));
}

void RVCSWController::apply(CommandResult result) {
    if (result.cleaningCommand()) {
        executeCleaningCommand(*result.cleaningCommand());
    }

    for (const auto& command : result.movementCommands()) {
        executeMovementCommand(command);
    }

    startTimerIfNeeded(result.timerDuration());
}

void RVCSWController::executeMovementCommand(const MovementCommand& command) {
    switch (command.commandType()) {
    case MovementCommandType::MoveForward:
        drivingDevice_.moveForward();
        break;
    case MovementCommandType::MoveBackward:
        drivingDevice_.moveBackward();
        break;
    case MovementCommandType::TurnClockwise90:
    case MovementCommandType::TurnRight:
        drivingDevice_.turnRight();
        break;
    case MovementCommandType::TurnCounterClockwise90:
    case MovementCommandType::TurnLeft:
        drivingDevice_.turnLeft();
        break;
    case MovementCommandType::Stop:
        drivingDevice_.stop();
        break;
    }
}

void RVCSWController::executeCleaningCommand(const CleaningCommand& command) {
    cleaningDevice_.setCleaningPower(command.powerLevel());
}

void RVCSWController::startTimerIfNeeded(const std::optional<Duration>& duration) {
    if (!duration || duration->isZero()) {
        return;
    }

    time_.startTimer(*duration);
}

} // namespace rvc
