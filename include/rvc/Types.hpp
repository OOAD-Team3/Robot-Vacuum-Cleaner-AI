#pragma once

namespace rvc {

enum class MovementStatus {
    Cleaning,
    Rotating,
    AvoidingObstacle,
    Blocked,
    Stopped
};

enum class MovementCommandType {
    MoveForward,
    MoveBackward,
    TurnClockwise90,
    TurnCounterClockwise90,
    TurnLeft,
    TurnRight,
    Stop
};

enum class CleaningPowerLevel {
    Normal,
    Increased
};

enum class AvoidanceDirection {
    Left,
    Right
};

enum class RightProbeResult {
    Unknown,
    Open,
    Blocked
};

enum class BackObstacleInput {
    Clear,
    Blocked,
    Unknown
};

enum class TravelDirection {
    Forward,
    Backward
};

enum class RotationDirection {
    Clockwise,
    CounterClockwise
};

enum class RotationCause {
    Dust,
    Obstacle
};

enum class TargetSensor {
    Front,
    Back
};

class Duration {
public:
    constexpr Duration() = default;
    explicit constexpr Duration(int milliseconds) : milliseconds_(milliseconds) {}

    static constexpr Duration milliseconds(int value) { return Duration(value); }
    static constexpr Duration seconds(int value) { return Duration(value * 1000); }

    constexpr int inMilliseconds() const { return milliseconds_; }
    constexpr bool isZero() const { return milliseconds_ == 0; }

private:
    int milliseconds_{0};
};

} // namespace rvc
