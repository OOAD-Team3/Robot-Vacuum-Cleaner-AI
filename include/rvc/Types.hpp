#pragma once

namespace rvc {

enum class MovementStatus {
    Cleaning,
    AvoidingObstacle,
    Blocked,
    Stopped
};

enum class MovementCommandType {
    MoveForward,
    MoveBackward,
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
