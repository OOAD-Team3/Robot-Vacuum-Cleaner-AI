#include "rvc/SensorState.hpp"

namespace rvc {

void SensorState::updateFrontObstacle(bool frontObstacleDetected) {
    frontObstacleDetected_ = frontObstacleDetected;
}

void SensorState::updateBackObstacle(bool backObstacleDetected) {
    backObstacleDetected_ = backObstacleDetected;
}

void SensorState::updateLeftObstacle(bool leftObstacleDetected) {
    leftObstacleDetected_ = leftObstacleDetected;
}

void SensorState::updateObstacles(bool frontObstacleDetected, bool leftObstacleDetected) {
    frontObstacleDetected_ = frontObstacleDetected;
    backObstacleDetected_.reset();
    leftObstacleDetected_ = leftObstacleDetected;
}

void SensorState::updateObstacles(
    bool frontObstacleDetected,
    bool backObstacleDetected,
    bool leftObstacleDetected) {
    frontObstacleDetected_ = frontObstacleDetected;
    backObstacleDetected_ = backObstacleDetected;
    leftObstacleDetected_ = leftObstacleDetected;
}

void SensorState::updateSensorSnapshot(
    bool frontObstacleDetected,
    BackObstacleInput backObstacleDetected,
    bool dustDetected) {
    frontObstacleDetected_ = frontObstacleDetected;
    if (backObstacleDetected == BackObstacleInput::Unknown) {
        backObstacleDetected_.reset();
    } else {
        backObstacleDetected_ = backObstacleDetected == BackObstacleInput::Blocked;
    }
    dustDetected_ = dustDetected;
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

bool SensorState::isLeftObstacleDetected() const {
    return leftObstacleDetected_;
}

bool SensorState::isDustDetected() const {
    return dustDetected_;
}

bool SensorState::obstacleDetectedIn(TravelDirection direction) const {
    return direction == TravelDirection::Forward
        ? isFrontObstacleDetected()
        : isBackObstacleDetected();
}

bool SensorState::targetSensorIsClear(TargetSensor targetSensor) const {
    if (targetSensor == TargetSensor::Front) {
        return !isFrontObstacleDetected();
    }

    return backObstacleDetected_.has_value() && !*backObstacleDetected_;
}

bool SensorState::canMoveBackward() const {
    return backObstacleDetected_.has_value() && !*backObstacleDetected_;
}

} // namespace rvc
