#pragma once

#include "rvc/Types.hpp"

namespace rvc {

class DustResponse {
public:
    void start(CleaningPowerLevel powerLevel, Duration duration);
    void expire();

    bool isActive() const;
    CleaningPowerLevel currentPowerLevel() const;
    Duration duration() const;

private:
    bool active_{false};
    CleaningPowerLevel powerLevel_{CleaningPowerLevel::Normal};
    Duration duration_{};
};

} // namespace rvc

