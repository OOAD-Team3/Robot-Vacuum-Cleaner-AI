#pragma once

#include "rvc/CleaningPolicy.hpp"
#include "rvc/Commands.hpp"
#include "rvc/DustResponse.hpp"
#include "rvc/SensorState.hpp"
#include "rvc/Types.hpp"

namespace rvc {

class RightDirectionProbe {
public:
    void start();
    void resolveWithFrontObstacle(bool frontObstacleDetected);
    void clear();

    bool isActive() const;
    bool isOpen() const;
    bool isBlocked() const;
    bool restoreOriginalHeadingRequired() const;
    RightProbeResult result() const;

private:
    bool active_{false};
    RightProbeResult result_{RightProbeResult::Unknown};
    bool restoreOriginalHeadingRequired_{false};
};

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
    bool isRightProbeActive() const;

private:
    CommandResult normalCleaningResult();
    CommandResult maintainCurrentCleaningPower(CommandResult result);
    void clearThreeSideBlock();

    MovementStatus movementStatus_{MovementStatus::Stopped};
    CleaningPolicy policy_;
    RightDirectionProbe rightDirectionProbe_;
    bool threeSideBlockedConfirmed_{false};
    bool threeSideStopIssued_{false};
    DustResponse dustResponse_;
    bool dustResponsePending_{false};
};

} // namespace rvc
