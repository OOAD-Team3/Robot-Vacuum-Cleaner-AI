#pragma once

#include "rvc/Types.hpp"

namespace rvc {

class CleaningPolicy {
public:
    CleaningPolicy() = default;
    CleaningPolicy(
        AvoidanceDirectionPolicy avoidanceDirectionPolicy,
        CleaningPowerLevel increasedPowerLevel,
        Duration increasedPowerDuration);

    CleaningPowerLevel normalPowerLevel() const;
    CleaningPowerLevel increasedPowerLevel() const;
    Duration increasedPowerDuration() const;
    AvoidanceDirectionPolicy avoidanceDirectionPolicy() const;

private:
    AvoidanceDirectionPolicy avoidanceDirectionPolicy_{AvoidanceDirectionPolicy::LeftFirst};
    CleaningPowerLevel increasedPowerLevel_{CleaningPowerLevel::Increased};
    Duration increasedPowerDuration_{Duration::seconds(5)};
};

} // namespace rvc

