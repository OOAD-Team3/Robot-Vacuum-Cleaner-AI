#pragma once

#include "rvc/Devices.hpp"
#include "rvc/sim/ControllerStateSnapshot.hpp"

namespace rvc::sim {

class RecordingDrivingDevice final : public DrivingDevice {
public:
    void moveForward() override;
    void moveBackward() override;
    void turnLeft() override;
    void turnRight() override;
    void turn(AvoidanceDirection direction) override;
    void stop() override;

    void reset();
    DriveCommand lastCommand() const;

private:
    DriveCommand lastCommand_{DriveCommand::None};
};

class RecordingCleaningDevice final : public CleaningDevice {
public:
    void setCleaningPower(CleaningPowerLevel powerLevel) override;
    void keepCleaningPower(CleaningPowerLevel powerLevel) override;

    void reset();
    CleaningPowerState powerState() const;

private:
    CleaningPowerState powerState_{CleaningPowerState::Off};
};

class AppTime final : public Time {
public:
    void startTimer(Duration duration) override;

    void reset();
    void expireTimer();
    bool isTimerActive() const;
    Duration activeDuration() const;

private:
    bool timerActive_{false};
    Duration activeDuration_{};
};

} // namespace rvc::sim
