#pragma once

#include "rvc/Types.hpp"

namespace rvc {

class SensorInput {
public:
    virtual ~SensorInput() = default;

    virtual void reportFrontObstacleState(bool frontObstacleDetected) = 0;
    virtual void reportBackObstacleState(BackObstacleInput backObstacleDetected) = 0;
    virtual void reportLeftObstacleState(bool leftObstacleDetected) = 0;
    virtual void reportObstacleState(
        bool frontObstacleDetected,
        BackObstacleInput backObstacleDetected,
        bool leftObstacleDetected) = 0;
    virtual void reportSensorSnapshot(
        bool frontObstacleDetected,
        BackObstacleInput backObstacleDetected,
        bool dustDetected) = 0;
    virtual void reportDustDetected() = 0;
};

} // namespace rvc
