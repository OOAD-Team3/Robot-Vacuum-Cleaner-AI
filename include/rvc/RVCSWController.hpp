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
    void reportSensorSnapshot(
        bool frontObstacleDetected,
        BackObstacleInput backObstacleDetected,
        bool dustDetected) override;
    void reportDustDetected() override;
    void increasedPowerDurationExpired();

    MovementStatus movementStatus() const;
    TravelDirection travelDirection() const;
    bool isRotationActive() const;

private:
    void evaluateCurrentState();
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
