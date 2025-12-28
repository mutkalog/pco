#ifndef IREGISTRATIONSERVICE_H
#define IREGISTRATIONSERVICE_H

#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

class IRegistrationService
{
public:
    virtual ~IRegistrationService() = default;
    virtual uint64_t registerDevice(const json &body) = 0;
};

#endif // IREGISTRATIONSERVICE_H
