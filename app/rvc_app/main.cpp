#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include "rvc/net/TcpServer.hpp"
#include "rvc/protocol/CommandHandler.hpp"
#include "rvc/sim/RobotVacuumApplication.hpp"

namespace {

struct ServerConfig {
    std::string host{"127.0.0.1"};
    unsigned short port{9090};
};

bool parsePort(const std::string& text, unsigned short& port) {
    try {
        std::size_t parsedCharacters = 0;
        const auto value = std::stoi(text, &parsedCharacters);
        if (parsedCharacters != text.size() || value < 1 || value > 65535) {
            return false;
        }

        port = static_cast<unsigned short>(value);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parseListenArgument(const std::string& value, ServerConfig& config) {
    const auto separator = value.rfind(':');
    if (separator == std::string::npos || separator == 0 || separator + 1 >= value.size()) {
        return false;
    }

    unsigned short port = 0;
    if (!parsePort(value.substr(separator + 1), port)) {
        return false;
    }

    config.host = value.substr(0, separator);
    config.port = port;
    return true;
}

bool parseArguments(int argc, char* argv[], ServerConfig& config) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--listen") {
            if (index + 1 >= argc || !parseListenArgument(argv[++index], config)) {
                return false;
            }
            continue;
        }

        if (argument == "--host") {
            if (index + 1 >= argc) {
                return false;
            }
            config.host = argv[++index];
            continue;
        }

        if (argument == "--port") {
            if (index + 1 >= argc || !parsePort(argv[++index], config.port)) {
                return false;
            }
            continue;
        }

        return false;
    }

    return true;
}

void printUsage(const char* programName) {
    std::cerr
        << "Usage:\n"
        << "  " << programName << " --listen 127.0.0.1:9090\n"
        << "  " << programName << " --host 127.0.0.1 --port 9090\n"
        << "  " << programName << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    ServerConfig config;
    if (!parseArguments(argc, argv, config)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    try {
        rvc::sim::RobotVacuumApplication application;
        rvc::protocol::CommandHandler commandHandler{application};
        rvc::net::TcpServer server{
            config.host,
            config.port,
            [&commandHandler](const std::string& line) {
                const auto response = commandHandler.handleLine(line);
                return rvc::net::TcpServer::Response{response.text(), response.closeSession()};
            }};

        std::cout << "rvc_app listening on " << config.host << ':' << config.port << '\n';
        server.run();
    } catch (const std::exception& error) {
        std::cerr << "rvc_app failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
