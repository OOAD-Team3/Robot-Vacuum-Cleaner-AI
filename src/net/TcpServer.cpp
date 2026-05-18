#include "rvc/net/TcpServer.hpp"

#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include <asio.hpp>

namespace rvc::net {

namespace {

using asio::ip::tcp;

constexpr std::size_t kMaxCommandLineLength = 1024;

void writeResponse(tcp::socket& socket, const std::string& message) {
    asio::write(socket, asio::buffer(message));
}

TcpServer::Response handleLine(
    const std::string& line,
    const TcpServer::LineHandler& lineHandler,
    std::mutex& lineHandlerMutex) {
    std::lock_guard<std::mutex> lock{lineHandlerMutex};
    return lineHandler(line);
}

void handleSession(
    tcp::socket socket,
    const TcpServer::LineHandler& lineHandler,
    std::mutex& lineHandlerMutex) {
    std::string line;
    for (;;) {
        char ch = '\0';
        asio::error_code error;
        asio::read(socket, asio::buffer(&ch, 1), error);

        if (error == asio::error::eof || error == asio::error::connection_reset) {
            return;
        }

        if (error) {
            throw asio::system_error(error);
        }

        if (ch != '\n') {
            if (line.size() >= kMaxCommandLineLength) {
                writeResponse(socket, "ERR INVALID_ARGUMENT\n");
                return;
            }

            line.push_back(ch);
            continue;
        }

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const auto response = handleLine(line, lineHandler, lineHandlerMutex);
        writeResponse(socket, response.message());
        line.clear();

        if (response.closeSession()) {
            return;
        }
    }
}

} // namespace

TcpServer::Response::Response(std::string message, bool closeSession)
    : message_(std::move(message)), closeSession_(closeSession) {}

const std::string& TcpServer::Response::message() const {
    return message_;
}

bool TcpServer::Response::closeSession() const {
    return closeSession_;
}

TcpServer::TcpServer(std::string host, unsigned short port, LineHandler lineHandler)
    : host_(std::move(host)), port_(port), lineHandler_(std::move(lineHandler)) {}

void TcpServer::run() {
    asio::io_context ioContext;
    const auto address = asio::ip::make_address(host_);
    const tcp::endpoint endpoint{address, port_};
    tcp::acceptor acceptor{ioContext};
    acceptor.open(endpoint.protocol());
    acceptor.set_option(tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();
    auto lineHandlerMutex = std::make_shared<std::mutex>();

    for (;;) {
        tcp::socket socket{ioContext};
        acceptor.accept(socket);

        std::thread{
            [socket = std::move(socket), lineHandler = lineHandler_, lineHandlerMutex]() mutable {
                try {
                    handleSession(std::move(socket), lineHandler, *lineHandlerMutex);
                } catch (const std::exception& error) {
                    std::cerr << "Client session ended with error: " << error.what() << '\n';
                }
            }}
            .detach();
    }
}

} // namespace rvc::net
