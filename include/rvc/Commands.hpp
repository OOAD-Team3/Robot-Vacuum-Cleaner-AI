#pragma once

#include <optional>
#include <vector>

#include "rvc/Types.hpp"

namespace rvc {

class MovementCommand {
public:
    static MovementCommand create(MovementCommandType commandType);
    static MovementCommand createTurnCommand(AvoidanceDirection direction);

    MovementCommandType commandType() const;
    std::optional<AvoidanceDirection> direction() const;

private:
    MovementCommand(MovementCommandType commandType, std::optional<AvoidanceDirection> direction);

    MovementCommandType commandType_;
    std::optional<AvoidanceDirection> direction_;
};

class CleaningCommand {
public:
    static CleaningCommand create(CleaningPowerLevel powerLevel);

    CleaningPowerLevel powerLevel() const;

private:
    explicit CleaningCommand(CleaningPowerLevel powerLevel);

    CleaningPowerLevel powerLevel_;
};

class CommandResult {
public:
    static CommandResult none();

    CommandResult& withMovement(MovementCommand command);
    CommandResult& withCleaning(CleaningCommand command);
    CommandResult& withTimer(Duration duration);

    const std::vector<MovementCommand>& movementCommands() const;
    const std::optional<MovementCommand>& movementCommand() const;
    const std::optional<CleaningCommand>& cleaningCommand() const;
    const std::optional<Duration>& timerDuration() const;

private:
    std::vector<MovementCommand> movementCommands_;
    std::optional<MovementCommand> movementCommand_;
    std::optional<CleaningCommand> cleaningCommand_;
    std::optional<Duration> timerDuration_;
};

class AvoidanceDecision {
public:
    static AvoidanceDecision prepareDirectionDecision();

    void select(AvoidanceDirection direction);
    void selectByPolicy(AvoidanceDirectionPolicy policy);
    void evaluateBackwardRequired(const class SensorState& sensorState);
    void markBackwardRequired();
    void markNoAvailableDirection();

    bool hasSelectedDirection() const;
    std::optional<AvoidanceDirection> selectedDirection() const;
    bool backwardRequired() const;
    bool availableDirection() const;

private:
    std::optional<AvoidanceDirection> selectedDirection_;
    bool backwardRequired_{false};
    bool availableDirection_{true};
};

} // namespace rvc
