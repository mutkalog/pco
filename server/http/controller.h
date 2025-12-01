#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <httplib.h>

class Controller
{
public:
    virtual ~Controller() = default;
    virtual void registerRoute(httplib::Server& serv) = 0;
};

#endif // CONTROLLER_H
