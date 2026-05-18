#include "rvc/protocol/StateFormatter.hpp"

#include <sstream>

namespace rvc::protocol {

namespace {

const char* toText(MovementStatus status) {
    switch (status) {
    case MovementStatus::Cleaning:
        return "CLEANING";
    case MovementStatus::AvoidingObstacle:
        return "AVOIDING_OBSTACLE";
    case MovementStatus::Blocked:
        return "BLOCKED";
    case MovementStatus::Stopped:
        return "STOPPED";
    }

    return "STOPPED";
}

const char* toText(sim::DriveCommand command) {
    switch (command) {
    case sim::DriveCommand::None:
        return "NONE";
    case sim::DriveCommand::MoveForward:
        return "MOVE_FORWARD";
    case sim::DriveCommand::MoveBackward:
        return "MOVE_BACKWARD";
    case sim::DriveCommand::TurnLeft:
        return "TURN_LEFT";
    case sim::DriveCommand::TurnRight:
        return "TURN_RIGHT";
    case sim::DriveCommand::Stop:
        return "STOP";
    }

    return "NONE";
}

const char* toText(sim::CleaningPowerState state) {
    switch (state) {
    case sim::CleaningPowerState::Off:
        return "OFF";
    case sim::CleaningPowerState::Normal:
        return "NORMAL";
    case sim::CleaningPowerState::Increased:
        return "INCREASED";
    }

    return "OFF";
}

const char* toBit(bool value) {
    return value ? "1" : "0";
}

std::string toBackText(const std::optional<bool>& value) {
    if (!value) {
        return "UNKNOWN";
    }

    return *value ? "1" : "0";
}

} // namespace

std::string StateFormatter::format(const sim::ControllerStateSnapshot& snapshot) const {
    std::ostringstream response;
    response
        << "OK STATE"
        << " MOVEMENT=" << toText(snapshot.movementStatus())
        << " FRONT=" << toBit(snapshot.frontObstacleDetected())
        << " BACK=" << toBackText(snapshot.backObstacleDetected())
        << " LEFT=" << toBit(snapshot.leftObstacleDetected())
        << " RIGHT=" << toBit(snapshot.rightObstacleDetected())
        << " DUST=" << toBit(snapshot.dustDetected())
        << " DRIVE=" << toText(snapshot.driveCommand())
        << " CLEANING_POWER=" << toText(snapshot.cleaningPower())
        << " TIMER_ACTIVE=" << toBit(snapshot.timerActive());

    return response.str();
}

} // namespace rvc::protocol
