#pragma once

#include <functional>
#include <string>

namespace rvc::net {

class TcpServer {
public:
    class Response {
    public:
        Response(std::string message, bool closeSession);

        const std::string& message() const;
        bool closeSession() const;

    private:
        std::string message_;
        bool closeSession_{false};
    };

    using LineHandler = std::function<Response(const std::string&)>;
    using StartedHandler = std::function<void()>;

    TcpServer(
        std::string host,
        unsigned short port,
        LineHandler lineHandler,
        StartedHandler startedHandler = nullptr);

    void run();

private:
    std::string host_;
    unsigned short port_{0};
    LineHandler lineHandler_;
    StartedHandler startedHandler_;
};

} // namespace rvc::net
