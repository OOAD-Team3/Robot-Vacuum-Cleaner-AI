#pragma once

#include <optional>

namespace rvc {

class SideObstacleState {
public:
    SideObstacleState(bool leftBlocked, bool rightBlocked);

    bool leftBlocked() const;
    bool rightBlocked() const;
    bool bothBlocked() const;
    bool bothOpen() const;

private:
    bool leftBlocked_{false};
    bool rightBlocked_{false};
};

class SensorState {
public:
    void updateFrontObstacle(bool frontObstacleDetected);
    void updateBackObstacle(bool backObstacleDetected);
    void updateSideObstacles(bool leftObstacleDetected, bool rightObstacleDetected);
    void updateObstacles(bool frontObstacleDetected, bool leftObstacleDetected, bool rightObstacleDetected);
    void updateObstacles(
        bool frontObstacleDetected,
        bool backObstacleDetected,
        bool leftObstacleDetected,
        bool rightObstacleDetected);
    void clearBackObstacleState();
    void updateDustDetected(bool dustDetected);

    bool isFrontObstacleDetected() const;
    bool isBackObstacleDetected() const;
    bool isBackObstacleStateKnown() const;
    bool isDustDetected() const;
    bool isThreeSideBlocked() const;
    bool canMoveBackward() const;
    SideObstacleState sideObstacleState() const;

private:
    bool frontObstacleDetected_{false};
    std::optional<bool> backObstacleDetected_;
    bool leftObstacleDetected_{false};
    bool rightObstacleDetected_{false};
    bool dustDetected_{false};
};

} // namespace rvc
