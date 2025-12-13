#ifndef REGISTRATIONCONTROLLER_H
#define REGISTRATIONCONTROLLER_H

#include "controller.h"
#include "service/registrationservice.h"


class RegistrationController final : public Controller
{
public:
    RegistrationController(std::shared_ptr<ServerContext>& sc) : Controller(sc) {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    RegistrationService service_;
};

#endif // REGISTRATIONCONTROLLER_H
