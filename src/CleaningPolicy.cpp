#include "rvc/CleaningPolicy.hpp"

namespace rvc {

CleaningPolicy::CleaningPolicy(
    AvoidanceDirectionPolicy avoidanceDirectionPolicy,
    CleaningPowerLevel increasedPowerLevel,
    Duration increasedPowerDuration)
    : avoidanceDirectionPolicy_(avoidanceDirectionPolicy),
      increasedPowerLevel_(increasedPowerLevel),
      increasedPowerDuration_(increasedPowerDuration) {}

CleaningPowerLevel CleaningPolicy::normalPowerLevel() const {
    return CleaningPowerLevel::Normal;
}

CleaningPowerLevel CleaningPolicy::increasedPowerLevel() const {
    return increasedPowerLevel_;
}

Duration CleaningPolicy::increasedPowerDuration() const {
    return increasedPowerDuration_;
}

AvoidanceDirectionPolicy CleaningPolicy::avoidanceDirectionPolicy() const {
    return avoidanceDirectionPolicy_;
}

} // namespace rvc

