#include "rvc/AutomaticCleaning.hpp"

namespace rvc {

AutomaticCleaning::AutomaticCleaning(CleaningPolicy policy) : policy_(policy) {}

CommandResult AutomaticCleaning::handleSensorState(const SensorState& sensorState) {
    if (sensorState.isThreeSideBlocked()) {
        return handleThreeSideObstacle(sensorState);
    }

    if (sensorState.isFrontObstacleDetected()) {
        movementStatus_ = MovementStatus::AvoidingObstacle;
        return CommandResult::none().withMovement(MovementCommand::create(MovementCommandType::Stop));
    }

    return normalCleaningResult();
}

AvoidanceDecision AutomaticCleaning::selectAvoidanceDirection(const SensorState& sensorState) {
    auto decision = AvoidanceDecision::prepareDirectionDecision();
    const auto sideState = sensorState.sideObstacleState();

    if (sideState.bothBlocked()) {
        decision.markBackwardRequired();
        movementStatus_ = MovementStatus::Blocked;
        return decision;
    }

    if (sideState.leftBlocked()) {
        decision.select(AvoidanceDirection::Right);
        movementStatus_ = MovementStatus::AvoidingObstacle;
        return decision;
    }

    if (sideState.rightBlocked()) {
        decision.select(AvoidanceDirection::Left);
        movementStatus_ = MovementStatus::AvoidingObstacle;
        return decision;
    }

    decision.selectByPolicy(policy_.avoidanceDirectionPolicy());
    movementStatus_ = MovementStatus::AvoidingObstacle;
    return decision;
}

AvoidanceDecision AutomaticCleaning::selectAvoidanceDirectionByPolicy() {
    auto decision = AvoidanceDecision::prepareDirectionDecision();
    decision.selectByPolicy(policy_.avoidanceDirectionPolicy());
    movementStatus_ = MovementStatus::AvoidingObstacle;
    return decision;
}

CommandResult AutomaticCleaning::resumeAfterTurn(const SensorState& sensorState) {
    if (sensorState.isFrontObstacleDetected()) {
        movementStatus_ = MovementStatus::AvoidingObstacle;
        return CommandResult::none().withMovement(MovementCommand::create(MovementCommandType::Stop));
    }

    movementStatus_ = MovementStatus::Cleaning;
    auto result = CommandResult::none().withMovement(MovementCommand::create(MovementCommandType::MoveForward));
    return maintainCurrentCleaningPower(result);
}

CommandResult AutomaticCleaning::handleThreeSideObstacle(const SensorState& sensorState) {
    if (!sensorState.isThreeSideBlocked()) {
        return CommandResult::none();
    }

    if (!sensorState.isBackObstacleStateKnown()) {
        movementStatus_ = MovementStatus::Blocked;
        return CommandResult::none().withMovement(MovementCommand::create(MovementCommandType::Stop));
    }

    if (!sensorState.canMoveBackward()) {
        movementStatus_ = MovementStatus::Stopped;
        return CommandResult::none().withMovement(MovementCommand::create(MovementCommandType::Stop));
    }

    const auto wasAlreadyBlocked = movementStatus_ == MovementStatus::Blocked;
    movementStatus_ = MovementStatus::Blocked;

    auto result = CommandResult::none();
    if (!wasAlreadyBlocked) {
        result.withMovement(MovementCommand::create(MovementCommandType::Stop));
    }

    return result.withMovement(MovementCommand::create(MovementCommandType::MoveBackward));
}

CommandResult AutomaticCleaning::handleDustDetected(const SensorState& sensorState) {
    if (!sensorState.isDustDetected()) {
        return CommandResult::none();
    }

    const auto powerLevel = policy_.increasedPowerLevel();
    const auto duration = policy_.increasedPowerDuration();
    dustResponse_.start(powerLevel, duration);
    dustResponsePending_ = false;

    return CommandResult::none()
        .withCleaning(CleaningCommand::create(powerLevel))
        .withTimer(duration);
}

CommandResult AutomaticCleaning::handleObstacleWhileDustResponse(const SensorState& sensorState) {
    auto result = handleSensorState(sensorState);

    if (dustResponse_.isActive()) {
        result.withCleaning(CleaningCommand::create(dustResponse_.currentPowerLevel()));
    }

    return result;
}

CommandResult AutomaticCleaning::handleDustResponseTimeout() {
    if (!dustResponse_.isActive()) {
        return CommandResult::none();
    }

    dustResponse_.expire();
    dustResponsePending_ = false;
    return CommandResult::none().withCleaning(CleaningCommand::create(policy_.normalPowerLevel()));
}

void AutomaticCleaning::markDustResponsePending() {
    dustResponsePending_ = true;
}

void AutomaticCleaning::clearDustResponseState() {
    dustResponse_.expire();
    dustResponsePending_ = false;
}

void AutomaticCleaning::keepCurrentMovementStatus() {}

void AutomaticCleaning::changeMovementStatus(MovementStatus status) {
    movementStatus_ = status;
}

void AutomaticCleaning::keepMovementStatus(MovementStatus status) {
    if (movementStatus_ != status) {
        return;
    }
}

MovementStatus AutomaticCleaning::movementStatus() const {
    return movementStatus_;
}

bool AutomaticCleaning::isDustResponseActive() const {
    return dustResponse_.isActive();
}

bool AutomaticCleaning::isDustResponsePending() const {
    return dustResponsePending_;
}

CommandResult AutomaticCleaning::normalCleaningResult() {
    movementStatus_ = MovementStatus::Cleaning;

    auto result = CommandResult::none()
        .withMovement(MovementCommand::create(MovementCommandType::MoveForward));

    return maintainCurrentCleaningPower(result);
}

CommandResult AutomaticCleaning::maintainCurrentCleaningPower(CommandResult result) {
    if (dustResponse_.isActive()) {
        result.withCleaning(CleaningCommand::create(dustResponse_.currentPowerLevel()));
        return result;
    }

    result.withCleaning(CleaningCommand::create(policy_.normalPowerLevel()));
    return result;
}

} // namespace rvc
