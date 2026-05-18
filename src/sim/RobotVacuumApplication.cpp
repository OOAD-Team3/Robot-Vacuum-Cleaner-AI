#include "rvc/sim/RobotVacuumApplication.hpp"

namespace rvc::sim {

RobotVacuumApplication::RobotVacuumApplication() {
    createController();
}

void RobotVacuumApplication::reset() {
    drivingDevice_.reset();
    cleaningDevice_.reset();
    time_.reset();
    frontObstacleDetected_ = false;
    backObstacleDetected_.reset();
    leftObstacleDetected_ = false;
    rightObstacleDetected_ = false;
    dustDetected_ = false;
    createController();
}

void RobotVacuumApplication::setFrontObstacle(bool detected) {
    frontObstacleDetected_ = detected;
    controller_->reportFrontObstacleState(detected);
}

void RobotVacuumApplication::setBackObstacle(BackObstacleInput detected) {
    backObstacleDetected_ = toOptionalBackState(detected);

    if (!backObstacleDetected_) {
        controller_->reportBackObstacleStateUnknown();
        return;
    }

    controller_->reportBackObstacleState(*backObstacleDetected_);
}

void RobotVacuumApplication::setSideObstacles(bool leftDetected, bool rightDetected) {
    leftObstacleDetected_ = leftDetected;
    rightObstacleDetected_ = rightDetected;
    controller_->reportSideObstacleState(leftDetected, rightDetected);
}

void RobotVacuumApplication::setObstacleState(
    bool frontDetected,
    BackObstacleInput backDetected,
    bool leftDetected,
    bool rightDetected) {
    frontObstacleDetected_ = frontDetected;
    backObstacleDetected_ = toOptionalBackState(backDetected);
    leftObstacleDetected_ = leftDetected;
    rightObstacleDetected_ = rightDetected;

    if (backObstacleDetected_) {
        controller_->reportObstacleState(
            frontDetected,
            *backObstacleDetected_,
            leftDetected,
            rightDetected);
        return;
    }

    controller_->reportObstacleState(frontDetected, leftDetected, rightDetected);
}

void RobotVacuumApplication::reportDustDetected() {
    dustDetected_ = true;
    controller_->reportDustDetected();
}

bool RobotVacuumApplication::expirePowerTimer() {
    if (!time_.isTimerActive()) {
        return false;
    }

    time_.expireTimer();
    controller_->increasedPowerDurationExpired();
    dustDetected_ = false;
    return true;
}

ControllerStateSnapshot RobotVacuumApplication::snapshot() const {
    return ControllerStateSnapshot{
        controller_->movementStatus(),
        frontObstacleDetected_,
        backObstacleDetected_,
        leftObstacleDetected_,
        rightObstacleDetected_,
        dustDetected_,
        drivingDevice_.lastCommand(),
        cleaningDevice_.powerState(),
        time_.isTimerActive()};
}

void RobotVacuumApplication::createController() {
    controller_ = std::make_unique<RVCSWController>(drivingDevice_, cleaningDevice_, time_);
}

std::optional<bool> RobotVacuumApplication::toOptionalBackState(BackObstacleInput detected) const {
    switch (detected) {
    case BackObstacleInput::Clear:
        return false;
    case BackObstacleInput::Blocked:
        return true;
    case BackObstacleInput::Unknown:
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace rvc::sim
