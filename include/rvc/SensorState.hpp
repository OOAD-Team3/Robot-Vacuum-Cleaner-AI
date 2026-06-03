#pragma once

#include <optional>

namespace rvc {

class SensorState {
public:
    void updateFrontObstacle(bool frontObstacleDetected);
    void updateBackObstacle(bool backObstacleDetected);
    void updateLeftObstacle(bool leftObstacleDetected);
    void updateObstacles(bool frontObstacleDetected, bool leftObstacleDetected);
    void updateObstacles(bool frontObstacleDetected, bool backObstacleDetected, bool leftObstacleDetected);
    void clearBackObstacleState();
    void updateDustDetected(bool dustDetected);

    bool isFrontObstacleDetected() const;
    bool isBackObstacleDetected() const;
    bool isBackObstacleStateKnown() const;
    bool isLeftObstacleDetected() const;
    bool isDustDetected() const;
    bool canMoveBackward() const;

private:
    bool frontObstacleDetected_{false};
    std::optional<bool> backObstacleDetected_;
    bool leftObstacleDetected_{false};
    bool dustDetected_{false};
};

} // namespace rvc
