#ifndef SERVER_H
#define SERVER_H

#include "controller.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <unistd.h>


class Server
{
public:

    void setupRoutes();
    bool listen(const std::string& interface, int port, int socketFlags = 0);
    void setControllers(std::vector<std::unique_ptr<Controller> >&& controllers);
    void addController(std::unique_ptr<Controller>&& controller);

    static Server& instance();

private:
    Server();
    httplib::SSLServer serv_;
    std::vector<std::unique_ptr<Controller>> controllers_;
};

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

inline Server &Server::instance()
{
    static Server inst;
    return inst;
}


#endif // SERVER_H
