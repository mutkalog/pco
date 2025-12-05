#include "registrationservice.h"
#include "database.h"
#include <iostream>
#include <nlohmann/json.hpp>


using json = nlohmann::ordered_json;


uint64_t RegistrationService::registerDevice(const std::string &devType, const std::string &platform, const std::string &arch)
{
    if (devType.empty() || platform.empty() || arch.empty())
        throw std::runtime_error("Wrong parameters");

    pqxx::work txn(*conn_);
    pqxx::params params;
    params.append(devType);
    params.append(platform);
    params.append(arch);

    pqxx::result res = txn.exec(
        "INSERT INTO devices (device_type, platform, arch) "
        "VALUES ($1, $2, $3) "
        "RETURNING id;",
        params);

    txn.commit();

    if (res.empty())
    {
        throw std::system_error(std::error_code(0, std::generic_category()),
                            "Server error");
    }

    auto id = res[0]["id"].as<uint64_t>();
    std::cout << "Inserted release ID: " << id << std::endl;

    return id;
}
