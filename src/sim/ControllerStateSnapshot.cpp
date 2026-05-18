#include "rvc/sim/ControllerStateSnapshot.hpp"

namespace rvc::sim {

ControllerStateSnapshot::ControllerStateSnapshot(
    MovementStatus movementStatus,
    bool frontObstacleDetected,
    std::optional<bool> backObstacleDetected,
    bool leftObstacleDetected,
    bool rightObstacleDetected,
    bool dustDetected,
    DriveCommand driveCommand,
    CleaningPowerState cleaningPower,
    bool timerActive)
    : movementStatus_(movementStatus),
      frontObstacleDetected_(frontObstacleDetected),
      backObstacleDetected_(backObstacleDetected),
      leftObstacleDetected_(leftObstacleDetected),
      rightObstacleDetected_(rightObstacleDetected),
      dustDetected_(dustDetected),
      driveCommand_(driveCommand),
      cleaningPower_(cleaningPower),
      timerActive_(timerActive) {}

MovementStatus ControllerStateSnapshot::movementStatus() const {
    return movementStatus_;
}

bool ControllerStateSnapshot::frontObstacleDetected() const {
    return frontObstacleDetected_;
}

std::optional<bool> ControllerStateSnapshot::backObstacleDetected() const {
    return backObstacleDetected_;
}

bool ControllerStateSnapshot::leftObstacleDetected() const {
    return leftObstacleDetected_;
}

bool ControllerStateSnapshot::rightObstacleDetected() const {
    return rightObstacleDetected_;
}

bool ControllerStateSnapshot::dustDetected() const {
    return dustDetected_;
}

DriveCommand ControllerStateSnapshot::driveCommand() const {
    return driveCommand_;
}

CleaningPowerState ControllerStateSnapshot::cleaningPower() const {
    return cleaningPower_;
}

bool ControllerStateSnapshot::timerActive() const {
    return timerActive_;
}

} // namespace rvc::sim
