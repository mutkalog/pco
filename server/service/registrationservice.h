#ifndef REGISTRATIONSERVICE_H
#define REGISTRATIONSERVICE_H

#include <cstdint>
#include <string>

class RegistrationService
{
public:
    RegistrationService() = default;
    uint64_t registerDevice(const std::string &devType, const std::string &platform,
                        const std::string &arch);
};

#endif // REGISTRATIONSERVICE_H
