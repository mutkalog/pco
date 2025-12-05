#ifndef REGISTRATIONSERVICE_H
#define REGISTRATIONSERVICE_H

#include "database.h"
#include <cstdint>
#include <string>



class RegistrationService
{
public:
    RegistrationService() : conn_(Database::instance().getConnection()) {}
    uint64_t registerDevice(const std::string &devType, const std::string &platform,
                        const std::string &arch);
private:
    std::unique_ptr<pqxx::connection> conn_;
};

#endif // REGISTRATIONSERVICE_H
