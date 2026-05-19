#include "rvc/SensorState.hpp"

namespace rvc {

SideObstacleState::SideObstacleState(bool leftBlocked, bool rightBlocked)
    : leftBlocked_(leftBlocked), rightBlocked_(rightBlocked) {}

bool SideObstacleState::leftBlocked() const {
    return leftBlocked_;
}

bool SideObstacleState::rightBlocked() const {
    return rightBlocked_;
}

bool SideObstacleState::bothBlocked() const {
    return leftBlocked_ && rightBlocked_;
}

bool SideObstacleState::bothOpen() const {
    return !leftBlocked_ && !rightBlocked_;
}

void SensorState::updateFrontObstacle(bool frontObstacleDetected) {
    frontObstacleDetected_ = frontObstacleDetected;
}

void SensorState::updateBackObstacle(bool backObstacleDetected) {
    backObstacleDetected_ = backObstacleDetected;
}

void SensorState::updateSideObstacles(bool leftObstacleDetected, bool rightObstacleDetected) {
    leftObstacleDetected_ = leftObstacleDetected;
    rightObstacleDetected_ = rightObstacleDetected;
}

void SensorState::updateObstacles(
    bool frontObstacleDetected,
    bool leftObstacleDetected,
    bool rightObstacleDetected) {
    frontObstacleDetected_ = frontObstacleDetected;
    backObstacleDetected_.reset();
    leftObstacleDetected_ = leftObstacleDetected;
    rightObstacleDetected_ = rightObstacleDetected;
}

void SensorState::updateObstacles(
    bool frontObstacleDetected,
    bool backObstacleDetected,
    bool leftObstacleDetected,
    bool rightObstacleDetected) {
    frontObstacleDetected_ = frontObstacleDetected;
    backObstacleDetected_ = backObstacleDetected;
    leftObstacleDetected_ = leftObstacleDetected;
    rightObstacleDetected_ = rightObstacleDetected;
}

void SensorState::clearBackObstacleState() {
    backObstacleDetected_.reset();
}

void SensorState::updateDustDetected(bool dustDetected) {
    dustDetected_ = dustDetected;
}

bool SensorState::isFrontObstacleDetected() const {
    return frontObstacleDetected_;
}

bool SensorState::isBackObstacleDetected() const {
    return backObstacleDetected_.value_or(false);
}

bool SensorState::isBackObstacleStateKnown() const {
    return backObstacleDetected_.has_value();
}

bool SensorState::isDustDetected() const {
    return dustDetected_;
}

bool SensorState::isThreeSideBlocked() const {
    return frontObstacleDetected_ && leftObstacleDetected_ && rightObstacleDetected_;
}

bool SensorState::canMoveBackward() const {
    return backObstacleDetected_.has_value() && !*backObstacleDetected_;
}

SideObstacleState SensorState::sideObstacleState() const {
    return SideObstacleState(leftObstacleDetected_, rightObstacleDetected_);
}

} // namespace rvc
