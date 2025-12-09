#ifndef REGISTRATIONSERVICE_H
#define REGISTRATIONSERVICE_H

#include "connectionspull.h"
#include <cstdint>
#include <nlohmann/json.hpp>


using json = nlohmann::ordered_json;

class RegistrationService
{
public:
    uint64_t registerDevice(const json &body);

private:
    ConnectionsPool cp_;
};

#endif // REGISTRATIONSERVICE_H
