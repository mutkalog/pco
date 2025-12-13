#include "reportservice.h"
#include "database.h"
#include <iostream>

namespace {
constexpr std::string_view CHECK_DEVICE_SQL =
    "SELECT id FROM devices WHERE id = $1";
}


void ReportService::parseReport(std::shared_ptr<ServerContext>& sc, const json &report)
{
    auto conn = cp_.acquire();
    try
    {
        uint64_t deviceId = report["id"];
        int      status   = report["error"]["code"];
        {
            auto &staging = sc->staging;

            std::lock_guard<std::mutex> lg(staging.updates.mtx);
            static_cast<void>(lg);

            pqxx::work txn(*conn);
            pqxx::params params;
            params.append(deviceId);

            pqxx::result dres = txn.exec(CHECK_DEVICE_SQL, params);

            if (dres.empty() == true || dres[0][0].as<uint64_t>() != deviceId)
                throw std::runtime_error("Cannot find such device id");

            auto updateInfoIt = staging.updates.devToReleaseMap.find(deviceId);

            if (updateInfoIt == staging.updates.devToReleaseMap.end())
            {
                std::cout << "Received report for update,"
                             " that already marked as failed"
                          << std::endl;
                return;
            }

            updateInfoIt->second.finished = true;
            updateInfoIt->second.status   = status;
            updateInfoIt->second.report   = report.dump();
        }

        cp_.release(std::move(conn));
    }
    catch (...)
    {
        cp_.release(std::move(conn));
        throw;
    }
}
