#pragma once

#include <memory>
#include <optional>

#include "rvc/RVCSWController.hpp"
#include "rvc/Types.hpp"
#include "rvc/sim/ControllerStateSnapshot.hpp"
#include "rvc/sim/RecordingDevices.hpp"

namespace rvc::sim {

class RobotVacuumApplication {
public:
    RobotVacuumApplication();

    void reset();
    void setFrontObstacle(bool detected);
    void setBackObstacle(rvc::BackObstacleInput detected);
    void setLeftObstacle(bool detected);
    void setObstacleState(
        bool frontDetected,
        rvc::BackObstacleInput backDetected,
        bool leftDetected);
    void setSensorSnapshot(
        bool frontDetected,
        rvc::BackObstacleInput backDetected,
        bool dustDetected);
    void reportDustDetected();
    bool expirePowerTimer();

    ControllerStateSnapshot snapshot() const;

private:
    void createController();
    std::optional<bool> toOptionalBackState(rvc::BackObstacleInput detected) const;

    RecordingDrivingDevice drivingDevice_;
    RecordingCleaningDevice cleaningDevice_;
    AppTime time_;
    std::unique_ptr<RVCSWController> controller_;
    bool frontObstacleDetected_{false};
    std::optional<bool> backObstacleDetected_;
    bool leftObstacleDetected_{false};
    bool dustDetected_{false};
};

} // namespace rvc::sim
