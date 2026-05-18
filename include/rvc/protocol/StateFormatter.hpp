#pragma once

#include <string>

#include "rvc/sim/ControllerStateSnapshot.hpp"

namespace rvc::protocol {

class StateFormatter {
public:
    std::string format(const sim::ControllerStateSnapshot& snapshot) const;
};

} // namespace rvc::protocol
