#include "registrationservice.h"
#include "database.h"
#include <iostream>



uint64_t RegistrationService::registerDevice(const json& body)
{
    auto conn = cp_.acquire();
    try
    {
        std::string type     = body["type"];
        std::string arch     = body["arch"];
        std::string platform = body["platform"];
        int         interval = body["pollingInterval"];

        if (type.empty() || platform.empty() || arch.empty())
            throw std::runtime_error("Wrong parameters");

        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(type);
        params.append(platform);
        params.append(arch);
        params.append(interval);

        pqxx::result res = txn.exec(
            "INSERT INTO devices (device_type, platform, arch, poling_interval) "
            "VALUES ($1, $2, $3, $4) "
            "RETURNING id;",
            params);

        txn.commit();

        if (res.empty())
        {
            throw std::system_error(std::error_code(0, std::generic_category()),
                                "Server error");
        }

        auto id = res[0]["id"].as<uint64_t>();
        std::cout << "Inserted device ID: " << id << std::endl;

        cp_.release(std::move(conn));

        return id;
    }
    catch (...)
    {
        cp_.release(std::move(conn));
        throw;
    }
}
