#pragma once

#include <optional>

#include "rvc/AutomaticCleaning.hpp"
#include "rvc/Devices.hpp"
#include "rvc/SensorState.hpp"

namespace rvc {

class RVCSWController {
public:
    RVCSWController(DrivingDevice& drivingDevice, CleaningDevice& cleaningDevice, Time& time);
    RVCSWController(
        DrivingDevice& drivingDevice,
        CleaningDevice& cleaningDevice,
        Time& time,
        AutomaticCleaning automaticCleaning);

    void reportFrontObstacleState(bool frontObstacleDetected);
    void reportBackObstacleState(bool backObstacleDetected);
    void reportSideObstacleState(bool leftObstacleDetected, bool rightObstacleDetected);
    void reportObstacleState(bool frontObstacleDetected, bool leftObstacleDetected, bool rightObstacleDetected);
    void reportObstacleState(
        bool frontObstacleDetected,
        bool backObstacleDetected,
        bool leftObstacleDetected,
        bool rightObstacleDetected);
    void reportDustDetected();
    void increasedPowerDurationExpired();

    const SensorState& sensorState() const;
    MovementStatus movementStatus() const;

private:
    void apply(CommandResult result);
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
