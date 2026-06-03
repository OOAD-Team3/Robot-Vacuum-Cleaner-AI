#include "rvc/AutomaticCleaning.hpp"

namespace rvc {

void RightDirectionProbe::start() {
    active_ = true;
    result_ = RightProbeResult::Unknown;
    restoreOriginalHeadingRequired_ = false;
}

void RightDirectionProbe::resolveWithFrontObstacle(bool frontObstacleDetected) {
    active_ = false;
    result_ = frontObstacleDetected ? RightProbeResult::Blocked : RightProbeResult::Open;
    restoreOriginalHeadingRequired_ = frontObstacleDetected;
}

void RightDirectionProbe::clear() {
    active_ = false;
    result_ = RightProbeResult::Unknown;
    restoreOriginalHeadingRequired_ = false;
}

bool RightDirectionProbe::isActive() const {
    return active_;
}

bool RightDirectionProbe::isOpen() const {
    return result_ == RightProbeResult::Open;
}

bool RightDirectionProbe::isBlocked() const {
    return result_ == RightProbeResult::Blocked;
}

bool RightDirectionProbe::restoreOriginalHeadingRequired() const {
    return restoreOriginalHeadingRequired_;
}

RightProbeResult RightDirectionProbe::result() const {
    return result_;
}

AutomaticCleaning::AutomaticCleaning(CleaningPolicy policy) : policy_(policy) {}

CommandResult AutomaticCleaning::handleSensorState(const SensorState& sensorState) {
    if (sensorState.isFrontObstacleDetected()) {
        movementStatus_ = MovementStatus::AvoidingObstacle;
        return CommandResult::none().withMovement(MovementCommand::create(MovementCommandType::Stop));
    }

    return normalCleaningResult();
}

AvoidanceDecision AutomaticCleaning::selectAvoidanceDirection(const SensorState& sensorState) {
    auto decision = AvoidanceDecision::prepareDirectionDecision();

    if (rightDirectionProbe_.isActive()) {
        rightDirectionProbe_.resolveWithFrontObstacle(sensorState.isFrontObstacleDetected());

        if (rightDirectionProbe_.isOpen()) {
            decision.select(AvoidanceDirection::Right);
            clearThreeSideBlock();
            movementStatus_ = MovementStatus::AvoidingObstacle;
            return decision;
        }

        threeSideBlockedConfirmed_ = true;
        threeSideStopIssued_ = false;
        decision.markBackwardRequired();
        movementStatus_ = MovementStatus::Blocked;
        return decision;
    }

    if (!sensorState.isLeftObstacleDetected()) {
        decision.select(AvoidanceDirection::Left);
        clearThreeSideBlock();
        movementStatus_ = MovementStatus::AvoidingObstacle;
        return decision;
    }

    clearThreeSideBlock();
    rightDirectionProbe_.start();
    decision.markRightProbeRequired();
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
    if (!threeSideBlockedConfirmed_) {
        return CommandResult::none();
    }

    if (!sensorState.isBackObstacleStateKnown()) {
        movementStatus_ = MovementStatus::Blocked;
        threeSideStopIssued_ = true;
        return CommandResult::none().withMovement(MovementCommand::create(MovementCommandType::Stop));
    }

    if (!sensorState.canMoveBackward()) {
        movementStatus_ = MovementStatus::Stopped;
        threeSideStopIssued_ = true;
        return CommandResult::none().withMovement(MovementCommand::create(MovementCommandType::Stop));
    }

    movementStatus_ = MovementStatus::Blocked;

    auto result = CommandResult::none();
    if (!threeSideStopIssued_) {
        result.withMovement(MovementCommand::create(MovementCommandType::Stop));
        threeSideStopIssued_ = true;
    }

    result.withMovement(MovementCommand::create(MovementCommandType::MoveBackward));
    clearThreeSideBlock();
    return result;
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
    auto result = threeSideBlockedConfirmed_
        ? handleThreeSideObstacle(sensorState)
        : handleSensorState(sensorState);

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

bool AutomaticCleaning::isRightProbeActive() const {
    return rightDirectionProbe_.isActive();
}

CommandResult AutomaticCleaning::normalCleaningResult() {
    movementStatus_ = MovementStatus::Cleaning;
    clearThreeSideBlock();

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

void AutomaticCleaning::clearThreeSideBlock() {
    threeSideBlockedConfirmed_ = false;
    threeSideStopIssued_ = false;
    if (!rightDirectionProbe_.isActive()) {
        rightDirectionProbe_.clear();
    }
}

} // namespace rvc
