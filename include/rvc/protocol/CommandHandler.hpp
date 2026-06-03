#pragma once

#include <string>

#include "rvc/Types.hpp"
#include "rvc/protocol/CommandParser.hpp"
#include "rvc/protocol/StateFormatter.hpp"
#include "rvc/sim/RobotVacuumApplication.hpp"

namespace rvc::protocol {

class CommandResponse {
public:
    CommandResponse(std::string text, bool closeSession);

    const std::string& text() const;
    bool closeSession() const;

private:
    std::string text_;
    bool closeSession_{false};
};

class CommandHandler {
public:
    explicit CommandHandler(sim::RobotVacuumApplication& application);

    CommandResponse handleLine(const std::string& line);

private:
    CommandResponse handleParsedCommand(const ParsedCommand& command);
    rvc::BackObstacleInput toApplicationBackValue(BackObstacleValue value) const;
    CommandResponse response(std::string text, bool closeSession = false) const;

    sim::RobotVacuumApplication& application_;
    CommandParser parser_;
    StateFormatter stateFormatter_;
};

} // namespace rvc::protocol
