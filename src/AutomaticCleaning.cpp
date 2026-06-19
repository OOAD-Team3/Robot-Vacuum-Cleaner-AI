#include "rvc/AutomaticCleaning.hpp"

namespace rvc {

AutomaticCleaning::AutomaticCleaning(CleaningPolicy policy) : policy_(policy) {}

CommandResult AutomaticCleaning::handleSensorState(const SensorState& sensorState) {
    if (rotationContext_ && rotationContext_->cause == RotationCause::Dust) {
        return continueRotation(sensorState);
    }

    if (sensorState.isDustDetected()) {
        return startRotation(RotationCause::Dust);
    }

    if (rotationContext_) {
        return continueRotation(sensorState);
    }

    if (sensorState.obstacleDetectedIn(travelDirection_)) {
        return startRotation(RotationCause::Obstacle);
    }

    return normalCleaningResult();
}

CommandResult AutomaticCleaning::handleDustResponseTimeout() {
    return CommandResult::none();
}

MovementStatus AutomaticCleaning::movementStatus() const {
    return movementStatus_;
}

TravelDirection AutomaticCleaning::travelDirection() const {
    return travelDirection_;
}

bool AutomaticCleaning::isRotationActive() const {
    return rotationContext_.has_value();
}

CommandResult AutomaticCleaning::startRotation(RotationCause cause) {
    rotationContext_ = createRotationContext(cause);
    movementStatus_ = MovementStatus::Rotating;

    return CommandResult::none()
        .withCleaning(CleaningCommand::create(rotationPowerLevel(cause)))
        .withMovement(turnCommand(rotationContext_->rotationDirection));
}

CommandResult AutomaticCleaning::continueRotation(const SensorState& sensorState) {
    if (!rotationContext_) {
        return normalCleaningResult();
    }

    if (!sensorState.targetSensorIsClear(rotationContext_->targetSensor)) {
        movementStatus_ = MovementStatus::Rotating;
        return CommandResult::none()
            .withCleaning(CleaningCommand::create(rotationPowerLevel(rotationContext_->cause)))
            .withMovement(turnCommand(rotationContext_->rotationDirection));
    }

    travelDirection_ = rotationContext_->nextTravelDirection;
    rotationContext_.reset();
    movementStatus_ = MovementStatus::Cleaning;

    return CommandResult::none()
        .withCleaning(CleaningCommand::create(policy_.normalPowerLevel()))
        .withMovement(travelCommand(travelDirection_));
}

CommandResult AutomaticCleaning::normalCleaningResult() {
    movementStatus_ = MovementStatus::Cleaning;

    return CommandResult::none()
        .withCleaning(CleaningCommand::create(policy_.normalPowerLevel()))
        .withMovement(travelCommand(travelDirection_));
}

MovementCommand AutomaticCleaning::turnCommand(RotationDirection direction) const {
    return MovementCommand::create(
        direction == RotationDirection::Clockwise
            ? MovementCommandType::TurnClockwise90
            : MovementCommandType::TurnCounterClockwise90);
}

MovementCommand AutomaticCleaning::travelCommand(TravelDirection direction) const {
    return MovementCommand::create(
        direction == TravelDirection::Forward
            ? MovementCommandType::MoveForward
            : MovementCommandType::MoveBackward);
}

CleaningPowerLevel AutomaticCleaning::rotationPowerLevel(RotationCause cause) const {
    return cause == RotationCause::Dust
        ? policy_.increasedPowerLevel()
        : policy_.normalPowerLevel();
}

AutomaticCleaning::RotationContext AutomaticCleaning::createRotationContext(RotationCause cause) const {
    if (travelDirection_ == TravelDirection::Forward) {
        return RotationContext{
            cause,
            RotationDirection::Clockwise,
            TargetSensor::Back,
            TravelDirection::Backward};
    }

    return RotationContext{
        cause,
        RotationDirection::CounterClockwise,
        TargetSensor::Front,
        TravelDirection::Forward};
}

} // namespace rvc
