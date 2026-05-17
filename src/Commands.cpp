#include "rvc/Commands.hpp"

#include "rvc/SensorState.hpp"

namespace rvc {

MovementCommand::MovementCommand(
    MovementCommandType commandType,
    std::optional<AvoidanceDirection> direction)
    : commandType_(commandType), direction_(direction) {}

MovementCommand MovementCommand::create(MovementCommandType commandType) {
    return MovementCommand(commandType, std::nullopt);
}

MovementCommand MovementCommand::createTurnCommand(AvoidanceDirection direction) {
    const auto commandType =
        direction == AvoidanceDirection::Left ? MovementCommandType::TurnLeft : MovementCommandType::TurnRight;
    return MovementCommand(commandType, direction);
}

MovementCommandType MovementCommand::commandType() const {
    return commandType_;
}

std::optional<AvoidanceDirection> MovementCommand::direction() const {
    return direction_;
}

CleaningCommand::CleaningCommand(CleaningPowerLevel powerLevel) : powerLevel_(powerLevel) {}

CleaningCommand CleaningCommand::create(CleaningPowerLevel powerLevel) {
    return CleaningCommand(powerLevel);
}

CleaningPowerLevel CleaningCommand::powerLevel() const {
    return powerLevel_;
}

CommandResult CommandResult::none() {
    return CommandResult{};
}

CommandResult& CommandResult::withMovement(MovementCommand command) {
    movementCommands_.push_back(command);
    movementCommand_ = command;
    return *this;
}

CommandResult& CommandResult::withCleaning(CleaningCommand command) {
    cleaningCommand_ = command;
    return *this;
}

CommandResult& CommandResult::withTimer(Duration duration) {
    timerDuration_ = duration;
    return *this;
}

const std::vector<MovementCommand>& CommandResult::movementCommands() const {
    return movementCommands_;
}

const std::optional<MovementCommand>& CommandResult::movementCommand() const {
    return movementCommand_;
}

const std::optional<CleaningCommand>& CommandResult::cleaningCommand() const {
    return cleaningCommand_;
}

const std::optional<Duration>& CommandResult::timerDuration() const {
    return timerDuration_;
}

AvoidanceDecision AvoidanceDecision::prepareDirectionDecision() {
    return AvoidanceDecision{};
}

void AvoidanceDecision::select(AvoidanceDirection direction) {
    selectedDirection_ = direction;
    backwardRequired_ = false;
    availableDirection_ = true;
}

void AvoidanceDecision::selectByPolicy(AvoidanceDirectionPolicy policy) {
    if (policy == AvoidanceDirectionPolicy::RightFirst) {
        select(AvoidanceDirection::Right);
        return;
    }

    select(AvoidanceDirection::Left);
}

void AvoidanceDecision::evaluateBackwardRequired(const SensorState& sensorState) {
    if (sensorState.isThreeSideBlocked()) {
        markBackwardRequired();
        return;
    }

    backwardRequired_ = false;
}

void AvoidanceDecision::markBackwardRequired() {
    selectedDirection_.reset();
    backwardRequired_ = true;
    availableDirection_ = false;
}

void AvoidanceDecision::markNoAvailableDirection() {
    selectedDirection_.reset();
    backwardRequired_ = false;
    availableDirection_ = false;
}

bool AvoidanceDecision::hasSelectedDirection() const {
    return selectedDirection_.has_value();
}

std::optional<AvoidanceDirection> AvoidanceDecision::selectedDirection() const {
    return selectedDirection_;
}

bool AvoidanceDecision::backwardRequired() const {
    return backwardRequired_;
}

bool AvoidanceDecision::availableDirection() const {
    return availableDirection_;
}

} // namespace rvc
