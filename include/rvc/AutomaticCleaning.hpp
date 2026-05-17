#pragma once

#include "rvc/CleaningPolicy.hpp"
#include "rvc/Commands.hpp"
#include "rvc/DustResponse.hpp"
#include "rvc/SensorState.hpp"
#include "rvc/Types.hpp"

namespace rvc {

class AutomaticCleaning {
public:
    explicit AutomaticCleaning(CleaningPolicy policy = CleaningPolicy{});

    CommandResult handleSensorState(const SensorState& sensorState);
    AvoidanceDecision selectAvoidanceDirection(const SensorState& sensorState);
    CommandResult resumeAfterTurn(const SensorState& sensorState);
    CommandResult handleThreeSideObstacle(const SensorState& sensorState);
    CommandResult handleDustDetected(const SensorState& sensorState);
    CommandResult handleObstacleWhileDustResponse(const SensorState& sensorState);
    CommandResult handleDustResponseTimeout();

    void markDustResponsePending();
    void clearDustResponseState();
    void keepCurrentMovementStatus();
    void changeMovementStatus(MovementStatus status);
    void keepMovementStatus(MovementStatus status);

    MovementStatus movementStatus() const;
    bool isDustResponseActive() const;
    bool isDustResponsePending() const;

private:
    CommandResult normalCleaningResult();
    CommandResult maintainCurrentCleaningPower(CommandResult result);

    MovementStatus movementStatus_{MovementStatus::Stopped};
    CleaningPolicy policy_;
    DustResponse dustResponse_;
    bool dustResponsePending_{false};
};

} // namespace rvc
