#include "rvc/RVCSWController.hpp"

namespace rvc {

namespace {

bool hasEffect(const CommandResult& result) {
    return !result.movementCommands().empty() ||
        result.cleaningCommand().has_value() ||
        result.timerDuration().has_value();
}

} // namespace

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

    if (applyRightProbeResultIfNeeded()) {
        return;
    }

    if (!frontObstacleDetected && automaticCleaning_.movementStatus() == MovementStatus::AvoidingObstacle) {
        apply(automaticCleaning_.resumeAfterTurn(sensorState_));
        applyPendingDustResponseIfCleaning();
        return;
    }

    if (!frontObstacleDetected && automaticCleaning_.movementStatus() == MovementStatus::Blocked) {
        applyAvoidanceDecisionFromCurrentState();
        return;
    }

    if (automaticCleaning_.isDustResponseActive()) {
        apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
        return;
    }

    apply(automaticCleaning_.handleSensorState(sensorState_));
    applyPendingDustResponseIfCleaning();
}

void RVCSWController::reportBackObstacleState(BackObstacleInput backObstacleDetected) {
    if (backObstacleDetected == BackObstacleInput::Unknown) {
        sensorState_.clearBackObstacleState();
    } else {
        sensorState_.updateBackObstacle(backObstacleDetected == BackObstacleInput::Blocked);
    }

    if (automaticCleaning_.isDustResponseActive()) {
        apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
        return;
    }

    apply(automaticCleaning_.handleThreeSideObstacle(sensorState_));
}

void RVCSWController::reportLeftObstacleState(bool leftObstacleDetected) {
    sensorState_.updateLeftObstacle(leftObstacleDetected);

    if (automaticCleaning_.isRightProbeActive()) {
        return;
    }

    if (automaticCleaning_.movementStatus() != MovementStatus::AvoidingObstacle &&
        automaticCleaning_.movementStatus() != MovementStatus::Blocked) {
        return;
    }

    applyAvoidanceDecisionFromCurrentState();
}

void RVCSWController::reportObstacleState(
    bool frontObstacleDetected,
    BackObstacleInput backObstacleDetected,
    bool leftObstacleDetected) {
    if (backObstacleDetected == BackObstacleInput::Unknown) {
        sensorState_.updateObstacles(frontObstacleDetected, leftObstacleDetected);
    } else {
        sensorState_.updateObstacles(
            frontObstacleDetected,
            backObstacleDetected == BackObstacleInput::Blocked,
            leftObstacleDetected);
    }

    if (applyRightProbeResultIfNeeded()) {
        return;
    }

    if (automaticCleaning_.movementStatus() == MovementStatus::Blocked) {
        auto result = automaticCleaning_.isDustResponseActive()
            ? automaticCleaning_.handleObstacleWhileDustResponse(sensorState_)
            : automaticCleaning_.handleThreeSideObstacle(sensorState_);
        if (hasEffect(result)) {
            apply(result);
            return;
        }
    }

    if (sensorState_.isFrontObstacleDetected()) {
        if (automaticCleaning_.movementStatus() == MovementStatus::AvoidingObstacle ||
            automaticCleaning_.movementStatus() == MovementStatus::Blocked) {
            applyAvoidanceDecisionFromCurrentState();
            return;
        }

        if (automaticCleaning_.isDustResponseActive()) {
            apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
            return;
        }

        apply(automaticCleaning_.handleSensorState(sensorState_));
        return;
    }

    if (automaticCleaning_.movementStatus() == MovementStatus::AvoidingObstacle) {
        apply(automaticCleaning_.resumeAfterTurn(sensorState_));
        applyPendingDustResponseIfCleaning();
        return;
    }

    if (automaticCleaning_.movementStatus() == MovementStatus::Blocked) {
        applyAvoidanceDecisionFromCurrentState();
        return;
    }

    if (automaticCleaning_.isDustResponseActive()) {
        apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
        return;
    }

    apply(automaticCleaning_.handleSensorState(sensorState_));
    applyPendingDustResponseIfCleaning();
}

void RVCSWController::reportDustDetected() {
    sensorState_.updateDustDetected(true);

    if (automaticCleaning_.movementStatus() != MovementStatus::Cleaning) {
        automaticCleaning_.markDustResponsePending();
        return;
    }

    applyInitialDustResponse(automaticCleaning_.handleDustDetected(sensorState_));
}

void RVCSWController::increasedPowerDurationExpired() {
    apply(automaticCleaning_.handleDustResponseTimeout());
}

MovementStatus RVCSWController::movementStatus() const {
    return automaticCleaning_.movementStatus();
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

void RVCSWController::applyInitialDustResponse(CommandResult result) {
    for (const auto& command : result.movementCommands()) {
        executeMovementCommand(command);
    }

    if (result.cleaningCommand()) {
        cleaningDevice_.setCleaningPower(result.cleaningCommand()->powerLevel());
    }

    startTimerIfNeeded(result.timerDuration());
}

void RVCSWController::applyPendingDustResponseIfCleaning() {
    if (!automaticCleaning_.isDustResponsePending() ||
        automaticCleaning_.movementStatus() != MovementStatus::Cleaning) {
        return;
    }

    applyInitialDustResponse(automaticCleaning_.handleDustDetected(sensorState_));
}

bool RVCSWController::applyRightProbeResultIfNeeded() {
    if (!automaticCleaning_.isRightProbeActive()) {
        return false;
    }

    const auto decision = automaticCleaning_.selectAvoidanceDirection(sensorState_);
    if (decision.backwardRequired()) {
        apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(AvoidanceDirection::Left)));
        return true;
    }

    if (decision.hasSelectedDirection() && *decision.selectedDirection() == AvoidanceDirection::Right) {
        apply(automaticCleaning_.resumeAfterTurn(sensorState_));
        applyPendingDustResponseIfCleaning();
        return true;
    }

    return true;
}

bool RVCSWController::applyAvoidanceDecisionFromCurrentState() {
    const auto decision = automaticCleaning_.selectAvoidanceDirection(sensorState_);
    applyAvoidanceDecision(decision);
    return decision.rightProbeRequired() || decision.backwardRequired() || decision.hasSelectedDirection();
}

void RVCSWController::applyAvoidanceDecision(const AvoidanceDecision& decision) {
    if (decision.rightProbeRequired()) {
        apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(AvoidanceDirection::Right)));
        return;
    }

    if (decision.backwardRequired() || !decision.hasSelectedDirection()) {
        return;
    }

    apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(*decision.selectedDirection())));
}

void RVCSWController::executeMovementCommand(const MovementCommand& command) {
    switch (command.commandType()) {
    case MovementCommandType::MoveForward:
        drivingDevice_.moveForward();
        break;
    case MovementCommandType::MoveBackward:
        drivingDevice_.moveBackward();
        break;
    case MovementCommandType::TurnLeft:
        drivingDevice_.turnLeft();
        break;
    case MovementCommandType::TurnRight:
        drivingDevice_.turnRight();
        break;
    case MovementCommandType::Stop:
        drivingDevice_.stop();
        break;
    }
}

void RVCSWController::executeCleaningCommand(const CleaningCommand& command) {
    if (automaticCleaning_.isDustResponseActive() && command.powerLevel() == CleaningPowerLevel::Increased) {
        cleaningDevice_.keepCleaningPower(command.powerLevel());
        return;
    }

    cleaningDevice_.setCleaningPower(command.powerLevel());
}

void RVCSWController::startTimerIfNeeded(const std::optional<Duration>& duration) {
    if (!duration || duration->isZero()) {
        return;
    }

    time_.startTimer(*duration);
}

} // namespace rvc
