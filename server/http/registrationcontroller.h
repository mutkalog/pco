#ifndef REGISTRATIONCONTROLLER_H
#define REGISTRATIONCONTROLLER_H

#include "controller.h"


class RegistrationController final : public Controller
{
public:
    RegistrationController() : Controller() {}
    virtual void registerRoute(httplib::Server& serv) override;
};

#endif // REGISTRATIONCONTROLLER_H
