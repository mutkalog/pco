#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <httplib.h>
#include "servercontext.h"

class Controller
{
public:
    Controller(std::shared_ptr<ServerContext>& sc) : sc_(sc) {}
    virtual ~Controller() = default;
    virtual void registerRoute(httplib::Server& serv) = 0;

protected:
    std::shared_ptr<ServerContext> sc_;
};

#endif // CONTROLLER_H
