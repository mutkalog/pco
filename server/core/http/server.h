#ifndef SERVER_H
#define SERVER_H

#include "core/http/controller.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <unistd.h>


class Server
{
public:
    Server(const std::string& ca, const std::string& cert, const std::string& key);

    void setupRoutes();
    bool listen(const std::string& interface, int port, int socketFlags = 0);
    void setControllers(std::vector<std::unique_ptr<Controller> >&& controllers);
    void addController(std::unique_ptr<Controller>&& controller);

private:
    httplib::SSLServer serv_;
    std::vector<std::unique_ptr<Controller>> controllers_;
};


inline Server::Server(const std::string &ca, const std::string &cert, const std::string &key)
    : serv_(cert.c_str(), key.c_str(), ca.c_str())
{
    if (!serv_.is_valid())
    {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to initialize SSL server");
    }
}


inline void Server::setupRoutes()
{
    for (const auto& cont : controllers_)
    {
        cont->registerRoute(serv_);
    }
}


inline bool Server::listen(const std::string &interface, int port, int socketFlags)
{
    return serv_.listen(interface, port, socketFlags);
}


inline void Server::setControllers(std::vector<std::unique_ptr<Controller> > &&controllers)
{
    controllers_ = std::move(controllers);
}


inline void Server::addController(std::unique_ptr<Controller> &&controller)
{
    controllers_.push_back(std::move(controller));
}


#endif // SERVER_H
