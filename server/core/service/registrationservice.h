#ifndef REGISTRATIONSERVICE_H
#define REGISTRATIONSERVICE_H

#include "core/connectionspull.h"
#include "core/service/interfaces/iregistrationservice.h"


class RegistrationService : public IRegistrationService
{
    friend class RegistrationServiceTest;

public:
    uint64_t registerDevice(const json &body) override;

private:
    ConnectionsPool cp_;
};

#endif // REGISTRATIONSERVICE_H
