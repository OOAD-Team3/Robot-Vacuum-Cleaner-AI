#pragma once

#include <optional>

#include "rvc/Types.hpp"

namespace rvc::sim {

enum class DriveCommand {
    None,
    MoveForward,
    MoveBackward,
    TurnClockwise90,
    TurnCounterClockwise90,
    TurnLeft,
    TurnRight,
    Stop
};

enum class CleaningPowerState {
    Off,
    Normal,
    Increased,
    Boost = Increased
};

class ControllerStateSnapshot {
public:
    ControllerStateSnapshot(
        MovementStatus movementStatus,
        TravelDirection travelDirection,
        bool rotationActive,
        bool frontObstacleDetected,
        std::optional<bool> backObstacleDetected,
        bool leftObstacleDetected,
        bool dustDetected,
        DriveCommand driveCommand,
        CleaningPowerState cleaningPower,
        bool timerActive);

    MovementStatus movementStatus() const;
    TravelDirection travelDirection() const;
    bool rotationActive() const;
    bool frontObstacleDetected() const;
    std::optional<bool> backObstacleDetected() const;
    bool leftObstacleDetected() const;
    bool dustDetected() const;
    DriveCommand driveCommand() const;
    CleaningPowerState cleaningPower() const;
    bool timerActive() const;

private:
    MovementStatus movementStatus_{MovementStatus::Stopped};
    TravelDirection travelDirection_{TravelDirection::Forward};
    bool rotationActive_{false};
    bool frontObstacleDetected_{false};
    std::optional<bool> backObstacleDetected_;
    bool leftObstacleDetected_{false};
    bool dustDetected_{false};
    DriveCommand driveCommand_{DriveCommand::None};
    CleaningPowerState cleaningPower_{CleaningPowerState::Off};
    bool timerActive_{false};
};

} // namespace rvc::sim
