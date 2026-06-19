#include "rvc/sim/RecordingDevices.hpp"

namespace rvc::sim {

namespace {

CleaningPowerState toCleaningPowerState(CleaningPowerLevel powerLevel) {
    return powerLevel == CleaningPowerLevel::Increased ? CleaningPowerState::Boost : CleaningPowerState::Normal;
}

} // namespace

void RecordingDrivingDevice::moveForward() {
    lastCommand_ = DriveCommand::MoveForward;
}

void RecordingDrivingDevice::moveBackward() {
    lastCommand_ = DriveCommand::MoveBackward;
}

void RecordingDrivingDevice::turnLeft() {
    lastCommand_ = DriveCommand::TurnCounterClockwise90;
}

void RecordingDrivingDevice::turnRight() {
    lastCommand_ = DriveCommand::TurnClockwise90;
}

void RecordingDrivingDevice::turn(AvoidanceDirection direction) {
    lastCommand_ = direction == AvoidanceDirection::Left
        ? DriveCommand::TurnCounterClockwise90
        : DriveCommand::TurnClockwise90;
}

void RecordingDrivingDevice::stop() {
    lastCommand_ = DriveCommand::Stop;
}

void RecordingDrivingDevice::reset() {
    lastCommand_ = DriveCommand::None;
}

DriveCommand RecordingDrivingDevice::lastCommand() const {
    return lastCommand_;
}

void RecordingCleaningDevice::setCleaningPower(CleaningPowerLevel powerLevel) {
    powerState_ = toCleaningPowerState(powerLevel);
}

void RecordingCleaningDevice::keepCleaningPower(CleaningPowerLevel powerLevel) {
    powerState_ = toCleaningPowerState(powerLevel);
}

void RecordingCleaningDevice::reset() {
    powerState_ = CleaningPowerState::Off;
}

CleaningPowerState RecordingCleaningDevice::powerState() const {
    return powerState_;
}

void AppTime::startTimer(Duration duration) {
    timerActive_ = true;
    activeDuration_ = duration;
}

void AppTime::reset() {
    timerActive_ = false;
    activeDuration_ = Duration{};
}

void AppTime::expireTimer() {
    timerActive_ = false;
    activeDuration_ = Duration{};
}

bool AppTime::isTimerActive() const {
    return timerActive_;
}

Duration AppTime::activeDuration() const {
    return activeDuration_;
}

} // namespace rvc::sim
