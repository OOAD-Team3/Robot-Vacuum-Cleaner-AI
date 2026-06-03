#pragma once

#include <optional>

#include "rvc/AutomaticCleaning.hpp"
#include "rvc/Devices.hpp"
#include "rvc/SensorInput.hpp"
#include "rvc/SensorState.hpp"

namespace rvc {

class RVCSWController : public SensorInput {
public:
    RVCSWController(DrivingDevice& drivingDevice, CleaningDevice& cleaningDevice, Time& time);
    RVCSWController(
        DrivingDevice& drivingDevice,
        CleaningDevice& cleaningDevice,
        Time& time,
        AutomaticCleaning automaticCleaning);

    void reportFrontObstacleState(bool frontObstacleDetected) override;
    void reportBackObstacleState(BackObstacleInput backObstacleDetected) override;
    void reportLeftObstacleState(bool leftObstacleDetected) override;
    void reportObstacleState(
        bool frontObstacleDetected,
        BackObstacleInput backObstacleDetected,
        bool leftObstacleDetected) override;
    void reportDustDetected() override;
    void increasedPowerDurationExpired();

    MovementStatus movementStatus() const;

private:
    void apply(CommandResult result);
    void applyInitialDustResponse(CommandResult result);
    void applyPendingDustResponseIfCleaning();
    bool applyRightProbeResultIfNeeded();
    bool applyAvoidanceDecisionFromCurrentState();
    void applyAvoidanceDecision(const AvoidanceDecision& decision);
    void executeMovementCommand(const MovementCommand& command);
    void executeCleaningCommand(const CleaningCommand& command);
    void startTimerIfNeeded(const std::optional<Duration>& duration);

    DrivingDevice& drivingDevice_;
    CleaningDevice& cleaningDevice_;
    Time& time_;
    SensorState sensorState_;
    AutomaticCleaning automaticCleaning_;
};

} // namespace rvc
