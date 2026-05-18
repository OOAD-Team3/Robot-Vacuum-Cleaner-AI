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

    if (!frontObstacleDetected && automaticCleaning_.movementStatus() == MovementStatus::AvoidingObstacle) {
        apply(automaticCleaning_.resumeAfterTurn(sensorState_));
        applyPendingDustResponseIfCleaning();
        return;
    }

    if (!frontObstacleDetected && automaticCleaning_.movementStatus() == MovementStatus::Blocked) {
        const auto decision = automaticCleaning_.selectAvoidanceDirectionByPolicy();
        if (decision.hasSelectedDirection()) {
            apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(*decision.selectedDirection())));
        }
        return;
    }

    if (automaticCleaning_.isDustResponseActive()) {
        apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
        return;
    }

    apply(automaticCleaning_.handleSensorState(sensorState_));
    applyPendingDustResponseIfCleaning();
}

void RVCSWController::reportBackObstacleState(bool backObstacleDetected) {
    sensorState_.updateBackObstacle(backObstacleDetected);

    if (sensorState_.isThreeSideBlocked()) {
        if (automaticCleaning_.isDustResponseActive()) {
            apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
            return;
        }

        apply(automaticCleaning_.handleThreeSideObstacle(sensorState_));
    }
}

void RVCSWController::reportBackObstacleStateUnknown() {
    sensorState_.clearBackObstacleState();

    if (sensorState_.isThreeSideBlocked()) {
        if (automaticCleaning_.isDustResponseActive()) {
            apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
            return;
        }

        apply(automaticCleaning_.handleThreeSideObstacle(sensorState_));
    }
}

void RVCSWController::reportSideObstacleState(bool leftObstacleDetected, bool rightObstacleDetected) {
    sensorState_.updateSideObstacles(leftObstacleDetected, rightObstacleDetected);

    if (automaticCleaning_.movementStatus() != MovementStatus::AvoidingObstacle &&
        automaticCleaning_.movementStatus() != MovementStatus::Blocked) {
        return;
    }

    const auto decision = automaticCleaning_.selectAvoidanceDirection(sensorState_);
    if (decision.backwardRequired()) {
        return;
    }

    if (!decision.hasSelectedDirection()) {
        return;
    }

    apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(*decision.selectedDirection())));
}

void RVCSWController::reportObstacleState(
    bool frontObstacleDetected,
    bool leftObstacleDetected,
    bool rightObstacleDetected) {
    sensorState_.updateObstacles(frontObstacleDetected, leftObstacleDetected, rightObstacleDetected);

    if (sensorState_.isThreeSideBlocked()) {
        if (automaticCleaning_.isDustResponseActive()) {
            apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
            return;
        }

        apply(automaticCleaning_.handleThreeSideObstacle(sensorState_));
        return;
    }

    if (sensorState_.isFrontObstacleDetected()) {
        if (automaticCleaning_.movementStatus() == MovementStatus::AvoidingObstacle) {
            const auto decision = automaticCleaning_.selectAvoidanceDirection(sensorState_);
            if (decision.backwardRequired()) {
                apply(automaticCleaning_.handleThreeSideObstacle(sensorState_));
                return;
            }

            if (decision.hasSelectedDirection()) {
                apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(*decision.selectedDirection())));
                return;
            }
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
        const auto decision = automaticCleaning_.selectAvoidanceDirection(sensorState_);
        if (decision.hasSelectedDirection()) {
            apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(*decision.selectedDirection())));
        }
        return;
    }

    if (automaticCleaning_.isDustResponseActive()) {
        apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
        return;
    }

    apply(automaticCleaning_.handleSensorState(sensorState_));
    applyPendingDustResponseIfCleaning();
}

void RVCSWController::reportObstacleState(
    bool frontObstacleDetected,
    bool backObstacleDetected,
    bool leftObstacleDetected,
    bool rightObstacleDetected) {
    sensorState_.updateObstacles(
        frontObstacleDetected,
        backObstacleDetected,
        leftObstacleDetected,
        rightObstacleDetected);

    if (sensorState_.isThreeSideBlocked()) {
        if (automaticCleaning_.isDustResponseActive()) {
            apply(automaticCleaning_.handleObstacleWhileDustResponse(sensorState_));
            return;
        }

        apply(automaticCleaning_.handleThreeSideObstacle(sensorState_));
        return;
    }

    if (sensorState_.isFrontObstacleDetected()) {
        if (automaticCleaning_.movementStatus() == MovementStatus::AvoidingObstacle) {
            const auto decision = automaticCleaning_.selectAvoidanceDirection(sensorState_);
            if (decision.backwardRequired()) {
                apply(automaticCleaning_.handleThreeSideObstacle(sensorState_));
                return;
            }

            if (decision.hasSelectedDirection()) {
                apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(*decision.selectedDirection())));
                return;
            }
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
        const auto decision = automaticCleaning_.selectAvoidanceDirection(sensorState_);
        if (decision.hasSelectedDirection()) {
            apply(CommandResult::none().withMovement(MovementCommand::createTurnCommand(*decision.selectedDirection())));
        }
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
