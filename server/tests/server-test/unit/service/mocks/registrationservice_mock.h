#ifndef REGISTRATIONSERVICE_MOCK_H
#define REGISTRATIONSERVICE_MOCK_H

#include <gmock/gmock.h>
#include "core/service/interfaces/iregistrationservice.h"


class MockRegistrationService : public IRegistrationService
{
public:
    MOCK_METHOD(uint64_t, registerDevice, (const json &body), (override));
};

#endif // REGISTRATIONSERVICE_MOCK_H
