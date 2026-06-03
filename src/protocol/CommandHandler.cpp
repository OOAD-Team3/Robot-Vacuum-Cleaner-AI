#include "rvc/protocol/CommandHandler.hpp"

#include <utility>

namespace rvc::protocol {

CommandResponse::CommandResponse(std::string text, bool closeSession)
    : text_(std::move(text)), closeSession_(closeSession) {}

const std::string& CommandResponse::text() const {
    return text_;
}

bool CommandResponse::closeSession() const {
    return closeSession_;
}

CommandHandler::CommandHandler(sim::RobotVacuumApplication& application)
    : application_(application) {}

CommandResponse CommandHandler::handleLine(const std::string& line) {
    const auto result = parser_.parse(line);
    if (!result.ok()) {
        return result.error() == ParseError::UnknownCommand
            ? response("ERR UNKNOWN_COMMAND")
            : response("ERR INVALID_ARGUMENT");
    }

    return handleParsedCommand(result.command());
}

CommandResponse CommandHandler::handleParsedCommand(const ParsedCommand& command) {
    switch (command.type()) {
    case CommandType::Ping:
        return response("OK PONG");
    case CommandType::Reset:
        application_.reset();
        return response("OK RESET");
    case CommandType::GetState:
        return response(stateFormatter_.format(application_.snapshot()));
    case CommandType::SetFront:
        application_.setFrontObstacle(command.frontObstacleDetected());
        return response("OK SET_FRONT");
    case CommandType::SetBack:
        application_.setBackObstacle(toApplicationBackValue(command.backObstacleDetected()));
        return response("OK SET_BACK");
    case CommandType::SetLeft:
        application_.setLeftObstacle(command.leftObstacleDetected());
        return response("OK SET_LEFT");
    case CommandType::SetObstacles:
        application_.setObstacleState(
            command.frontObstacleDetected(),
            toApplicationBackValue(command.backObstacleDetected()),
            command.leftObstacleDetected());
        return response("OK SET_OBSTACLES");
    case CommandType::DustDetected:
        application_.reportDustDetected();
        return response("OK DUST_DETECTED");
    case CommandType::PowerTimeout:
        if (!application_.expirePowerTimer()) {
            return response("ERR INVALID_STATE");
        }
        return response("OK POWER_TIMEOUT");
    case CommandType::Quit:
        return response("OK BYE", true);
    }

    return response("ERR UNKNOWN_COMMAND");
}

rvc::BackObstacleInput CommandHandler::toApplicationBackValue(BackObstacleValue value) const {
    switch (value) {
    case BackObstacleValue::Clear:
        return rvc::BackObstacleInput::Clear;
    case BackObstacleValue::Blocked:
        return rvc::BackObstacleInput::Blocked;
    case BackObstacleValue::Unknown:
        return rvc::BackObstacleInput::Unknown;
    }

    return rvc::BackObstacleInput::Unknown;
}

CommandResponse CommandHandler::response(std::string text, bool closeSession) const {
    text.push_back('\n');
    return CommandResponse{std::move(text), closeSession};
}

} // namespace rvc::protocol
