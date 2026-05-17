#include "rvc/DustResponse.hpp"

namespace rvc {

void DustResponse::start(CleaningPowerLevel powerLevel, Duration duration) {
    active_ = true;
    powerLevel_ = powerLevel;
    duration_ = duration;
}

void DustResponse::expire() {
    active_ = false;
    powerLevel_ = CleaningPowerLevel::Normal;
    duration_ = Duration{};
}

bool DustResponse::isActive() const {
    return active_;
}

CleaningPowerLevel DustResponse::currentPowerLevel() const {
    return powerLevel_;
}

Duration DustResponse::duration() const {
    return duration_;
}

} // namespace rvc

