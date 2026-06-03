#include "rvc/CleaningPolicy.hpp"

namespace rvc {

CleaningPolicy::CleaningPolicy(CleaningPowerLevel increasedPowerLevel, Duration increasedPowerDuration)
    : increasedPowerLevel_(increasedPowerLevel),
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

} // namespace rvc
