#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <httplib.h>
#include "servercontext.h"

class Controller
{
public:
    Controller(ServerContext* sc) : sc_(sc) {}
    virtual ~Controller() = default;
    virtual void registerRoute(httplib::Server& serv) = 0;

protected:
    ServerContext* sc_;
};

#endif // CONTROLLER_H
