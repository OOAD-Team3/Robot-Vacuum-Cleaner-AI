#pragma once

#include "rvc/Types.hpp"

namespace rvc {

class DrivingDevice {
public:
    virtual ~DrivingDevice() = default;

    virtual void moveForward() = 0;
    virtual void moveBackward() = 0;
    virtual void turnLeft() = 0;
    virtual void turnRight() = 0;
    virtual void turn(AvoidanceDirection direction) = 0;
    virtual void stop() = 0;
};

class CleaningDevice {
public:
    virtual ~CleaningDevice() = default;

    virtual void setCleaningPower(CleaningPowerLevel powerLevel) = 0;
    virtual void keepCleaningPower(CleaningPowerLevel powerLevel) = 0;
};

class Time {
public:
    virtual ~Time() = default;

    virtual void startTimer(Duration duration) = 0;
};

} // namespace rvc

