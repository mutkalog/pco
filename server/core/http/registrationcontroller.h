#ifndef REGISTRATIONCONTROLLER_H
#define REGISTRATIONCONTROLLER_H

#include "core/http/controller.h"
#include "core/service/interfaces/iregistrationservice.h"


class RegistrationController final : public Controller
{
public:
    RegistrationController(std::shared_ptr<ServerContext>& sc, std::unique_ptr<IRegistrationService> service)
        : Controller(sc), service_(std::move(service)) {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    std::unique_ptr<IRegistrationService> service_;
};

#endif // REGISTRATIONCONTROLLER_H
