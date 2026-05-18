#pragma once

#include <string>

namespace rvc::protocol {

enum class CommandType {
    Ping,
    Reset,
    GetState,
    SetFront,
    SetBack,
    SetSide,
    SetObstacles,
    DustDetected,
    PowerTimeout,
    Quit
};

enum class BackObstacleValue {
    Clear,
    Blocked,
    Unknown
};

enum class ParseError {
    None,
    UnknownCommand,
    InvalidArgument
};

class ParsedCommand {
public:
    static ParsedCommand simple(CommandType type);
    static ParsedCommand front(bool detected);
    static ParsedCommand back(BackObstacleValue detected);
    static ParsedCommand side(bool leftDetected, bool rightDetected);
    static ParsedCommand obstacles(
        bool frontDetected,
        BackObstacleValue backDetected,
        bool leftDetected,
        bool rightDetected);

    CommandType type() const;
    bool frontObstacleDetected() const;
    BackObstacleValue backObstacleDetected() const;
    bool leftObstacleDetected() const;
    bool rightObstacleDetected() const;

private:
    explicit ParsedCommand(CommandType type);

    CommandType type_;
    bool frontObstacleDetected_{false};
    BackObstacleValue backObstacleDetected_{BackObstacleValue::Unknown};
    bool leftObstacleDetected_{false};
    bool rightObstacleDetected_{false};
};

class ParseResult {
public:
    static ParseResult success(ParsedCommand command);
    static ParseResult failure(ParseError error);

    bool ok() const;
    const ParsedCommand& command() const;
    ParseError error() const;

private:
    ParseResult(ParsedCommand command, ParseError error, bool ok);

    ParsedCommand command_;
    ParseError error_{ParseError::None};
    bool ok_{false};
};

class CommandParser {
public:
    ParseResult parse(const std::string& line) const;
};

} // namespace rvc::protocol
