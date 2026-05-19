#pragma once

#include <optional>

#include "rvc/Types.hpp"

namespace rvc::sim {

enum class DriveCommand {
    None,
    MoveForward,
    MoveBackward,
    TurnLeft,
    TurnRight,
    Stop
};

enum class CleaningPowerState {
    Off,
    Normal,
    Increased
};

class ControllerStateSnapshot {
public:
    ControllerStateSnapshot(
        MovementStatus movementStatus,
        bool frontObstacleDetected,
        std::optional<bool> backObstacleDetected,
        bool leftObstacleDetected,
        bool rightObstacleDetected,
        bool dustDetected,
        DriveCommand driveCommand,
        CleaningPowerState cleaningPower,
        bool timerActive);

    MovementStatus movementStatus() const;
    bool frontObstacleDetected() const;
    std::optional<bool> backObstacleDetected() const;
    bool leftObstacleDetected() const;
    bool rightObstacleDetected() const;
    bool dustDetected() const;
    DriveCommand driveCommand() const;
    CleaningPowerState cleaningPower() const;
    bool timerActive() const;

private:
    MovementStatus movementStatus_{MovementStatus::Stopped};
    bool frontObstacleDetected_{false};
    std::optional<bool> backObstacleDetected_;
    bool leftObstacleDetected_{false};
    bool rightObstacleDetected_{false};
    bool dustDetected_{false};
    DriveCommand driveCommand_{DriveCommand::None};
    CleaningPowerState cleaningPower_{CleaningPowerState::Off};
    bool timerActive_{false};
};

} // namespace rvc::sim
