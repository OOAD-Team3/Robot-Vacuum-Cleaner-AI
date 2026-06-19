#include "rvc/sim/ControllerStateSnapshot.hpp"

namespace rvc::sim {

ControllerStateSnapshot::ControllerStateSnapshot(
    MovementStatus movementStatus,
    TravelDirection travelDirection,
    bool rotationActive,
    bool frontObstacleDetected,
    std::optional<bool> backObstacleDetected,
    bool leftObstacleDetected,
    bool dustDetected,
    DriveCommand driveCommand,
    CleaningPowerState cleaningPower,
    bool timerActive)
    : movementStatus_(movementStatus),
      travelDirection_(travelDirection),
      rotationActive_(rotationActive),
      frontObstacleDetected_(frontObstacleDetected),
      backObstacleDetected_(backObstacleDetected),
      leftObstacleDetected_(leftObstacleDetected),
      dustDetected_(dustDetected),
      driveCommand_(driveCommand),
      cleaningPower_(cleaningPower),
      timerActive_(timerActive) {}

MovementStatus ControllerStateSnapshot::movementStatus() const {
    return movementStatus_;
}

TravelDirection ControllerStateSnapshot::travelDirection() const {
    return travelDirection_;
}

bool ControllerStateSnapshot::rotationActive() const {
    return rotationActive_;
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
