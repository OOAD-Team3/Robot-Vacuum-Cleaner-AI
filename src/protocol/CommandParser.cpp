#include "rvc/protocol/CommandParser.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

namespace rvc::protocol {

namespace {

std::string trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (first >= last) {
        return {};
    }

    return std::string(first, last);
}

std::string uppercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::vector<std::string> splitWords(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> words;
    std::string word;

    while (stream >> word) {
        words.push_back(word);
    }

    return words;
}

std::optional<bool> parseBool(const std::string& value) {
    if (value == "0") {
        return false;
    }

    if (value == "1") {
        return true;
    }

    return std::nullopt;
}

std::optional<BackObstacleValue> parseBackValue(const std::string& value) {
    if (value == "0") {
        return BackObstacleValue::Clear;
    }

    if (value == "1") {
        return BackObstacleValue::Blocked;
    }

    if (uppercase(value) == "UNKNOWN") {
        return BackObstacleValue::Unknown;
    }

    return std::nullopt;
}

std::optional<std::pair<std::string, std::string>> parseKeyValue(const std::string& token) {
    const auto separator = token.find('=');
    if (separator == std::string::npos || separator == 0 || separator + 1 >= token.size()) {
        return std::nullopt;
    }

    return std::make_pair(
        uppercase(token.substr(0, separator)),
        token.substr(separator + 1));
}

bool collectKeyValues(
    const std::vector<std::string>& words,
    std::size_t firstArgument,
    std::map<std::string, std::string>& values) {
    for (std::size_t index = firstArgument; index < words.size(); ++index) {
        const auto keyValue = parseKeyValue(words[index]);
        if (!keyValue || values.find(keyValue->first) != values.end()) {
            return false;
        }

        values.emplace(keyValue->first, keyValue->second);
    }

    return true;
}

} // namespace

ParsedCommand ParsedCommand::simple(CommandType type) {
    return ParsedCommand{type};
}

ParsedCommand ParsedCommand::front(bool detected) {
    ParsedCommand command{CommandType::SetFront};
    command.frontObstacleDetected_ = detected;
    return command;
}

ParsedCommand ParsedCommand::back(BackObstacleValue detected) {
    ParsedCommand command{CommandType::SetBack};
    command.backObstacleDetected_ = detected;
    return command;
}

ParsedCommand ParsedCommand::left(bool detected) {
    ParsedCommand command{CommandType::SetLeft};
    command.leftObstacleDetected_ = detected;
    return command;
}

ParsedCommand ParsedCommand::obstacles(
    bool frontDetected,
    BackObstacleValue backDetected,
    bool leftDetected) {
    ParsedCommand command{CommandType::SetObstacles};
    command.frontObstacleDetected_ = frontDetected;
    command.backObstacleDetected_ = backDetected;
    command.leftObstacleDetected_ = leftDetected;
    return command;
}

ParsedCommand ParsedCommand::sensorSnapshot(
    bool frontDetected,
    BackObstacleValue backDetected,
    bool dustDetected) {
    ParsedCommand command{CommandType::SetSensorSnapshot};
    command.frontObstacleDetected_ = frontDetected;
    command.backObstacleDetected_ = backDetected;
    command.dustDetected_ = dustDetected;
    return command;
}

CommandType ParsedCommand::type() const {
    return type_;
}

bool ParsedCommand::frontObstacleDetected() const {
    return frontObstacleDetected_;
}

BackObstacleValue ParsedCommand::backObstacleDetected() const {
    return backObstacleDetected_;
}

bool ParsedCommand::leftObstacleDetected() const {
    return leftObstacleDetected_;
}

bool ParsedCommand::dustDetected() const {
    return dustDetected_;
}

ParsedCommand::ParsedCommand(CommandType type) : type_(type) {}

ParseResult ParseResult::success(ParsedCommand command) {
    return ParseResult{command, ParseError::None, true};
}

ParseResult ParseResult::failure(ParseError error) {
    return ParseResult{ParsedCommand::simple(CommandType::Ping), error, false};
}

bool ParseResult::ok() const {
    return ok_;
}

const ParsedCommand& ParseResult::command() const {
    return command_;
}

ParseError ParseResult::error() const {
    return error_;
}

ParseResult::ParseResult(ParsedCommand command, ParseError error, bool ok)
    : command_(command), error_(error), ok_(ok) {}

ParseResult CommandParser::parse(const std::string& line) const {
    const auto words = splitWords(trim(line));
    if (words.empty()) {
        return ParseResult::failure(ParseError::UnknownCommand);
    }

    const auto commandName = uppercase(words.front());

    if (commandName == "PING") {
        return words.size() == 1
            ? ParseResult::success(ParsedCommand::simple(CommandType::Ping))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "RESET") {
        return words.size() == 1
            ? ParseResult::success(ParsedCommand::simple(CommandType::Reset))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "GET_STATE") {
        return words.size() == 1
            ? ParseResult::success(ParsedCommand::simple(CommandType::GetState))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "DUST_DETECTED") {
        return words.size() == 1
            ? ParseResult::success(ParsedCommand::simple(CommandType::DustDetected))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "POWER_TIMEOUT") {
        return words.size() == 1
            ? ParseResult::success(ParsedCommand::simple(CommandType::PowerTimeout))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "QUIT") {
        return words.size() == 1
            ? ParseResult::success(ParsedCommand::simple(CommandType::Quit))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "SET_FRONT") {
        if (words.size() != 2) {
            return ParseResult::failure(ParseError::InvalidArgument);
        }

        const auto detected = parseBool(words[1]);
        return detected
            ? ParseResult::success(ParsedCommand::front(*detected))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "SET_BACK") {
        if (words.size() != 2) {
            return ParseResult::failure(ParseError::InvalidArgument);
        }

        const auto detected = parseBackValue(words[1]);
        return detected
            ? ParseResult::success(ParsedCommand::back(*detected))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "SET_LEFT") {
        if (words.size() != 2) {
            return ParseResult::failure(ParseError::InvalidArgument);
        }

        const auto left = parseBool(words[1]);
        return left
            ? ParseResult::success(ParsedCommand::left(*left))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "SET_OBSTACLES") {
        if (words.size() != 4) {
            return ParseResult::failure(ParseError::InvalidArgument);
        }

        std::map<std::string, std::string> values;
        if (!collectKeyValues(words, 1, values) || values.size() != 3 ||
            values.find("FRONT") == values.end() || values.find("BACK") == values.end() ||
            values.find("LEFT") == values.end()) {
            return ParseResult::failure(ParseError::InvalidArgument);
        }

        const auto front = parseBool(values["FRONT"]);
        const auto back = parseBackValue(values["BACK"]);
        const auto left = parseBool(values["LEFT"]);
        return front && back && left
            ? ParseResult::success(ParsedCommand::obstacles(*front, *back, *left))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    if (commandName == "SET_SENSOR_SNAPSHOT") {
        if (words.size() != 4) {
            return ParseResult::failure(ParseError::InvalidArgument);
        }

        std::map<std::string, std::string> values;
        if (!collectKeyValues(words, 1, values) || values.size() != 3 ||
            values.find("FRONT") == values.end() || values.find("BACK") == values.end() ||
            values.find("DUST") == values.end()) {
            return ParseResult::failure(ParseError::InvalidArgument);
        }

        const auto front = parseBool(values["FRONT"]);
        const auto back = parseBackValue(values["BACK"]);
        const auto dust = parseBool(values["DUST"]);
        return front && back && dust
            ? ParseResult::success(ParsedCommand::sensorSnapshot(*front, *back, *dust))
            : ParseResult::failure(ParseError::InvalidArgument);
    }

    return ParseResult::failure(ParseError::UnknownCommand);
}

} // namespace rvc::protocol
