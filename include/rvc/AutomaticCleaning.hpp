#pragma once

#include <optional>

#include "rvc/CleaningPolicy.hpp"
#include "rvc/Commands.hpp"
#include "rvc/SensorState.hpp"
#include "rvc/Types.hpp"

namespace rvc {

class AutomaticCleaning {
public:
    explicit AutomaticCleaning(CleaningPolicy policy = CleaningPolicy{});

    CommandResult handleSensorState(const SensorState& sensorState);
    CommandResult handleDustResponseTimeout();

    MovementStatus movementStatus() const;
    TravelDirection travelDirection() const;
    bool isRotationActive() const;

private:
    struct RotationContext {
        RotationCause cause;
        RotationDirection rotationDirection;
        TargetSensor targetSensor;
        TravelDirection nextTravelDirection;
    };

    CommandResult startRotation(RotationCause cause);
    CommandResult continueRotation(const SensorState& sensorState);
    CommandResult normalCleaningResult();
    MovementCommand turnCommand(RotationDirection direction) const;
    MovementCommand travelCommand(TravelDirection direction) const;
    CleaningPowerLevel rotationPowerLevel(RotationCause cause) const;
    RotationContext createRotationContext(RotationCause cause) const;

    MovementStatus movementStatus_{MovementStatus::Stopped};
    TravelDirection travelDirection_{TravelDirection::Forward};
    CleaningPolicy policy_;
    std::optional<RotationContext> rotationContext_;
};

} // namespace rvc
