#include "reportservice.h"
#include "database.h"
#include <iostream>

namespace {

constexpr std::string_view UPDATE_ASSIGMENT_STATUS_SQL =
    "UPDATE release_assignments "
    "SET status = $3 "
    "WHERE device_id = $1 AND release_id = $2";

constexpr std::string_view INSERT_REPORT_SQL =
    "INSERT INTO reports (device_id, release_id, status, body) "
    "VALUES ( "
    "    $1, "
    "    $2, "
    "    $3, "
    "    $4::jsonb "
    ") "
   "RETURNING id ";

}


void ReportService::parseReport(ServerContext* sc, const json &report)
{

    std::string type      = report["type"];
    std::string arch      = report["arch"];
    std::string platform  = report["platform"];
    uint64_t    deviceId  = report["id"];
    std::string version   = report["current_version"];
    int         status    = report["error"]["code"];
    uint64_t    releaseId;

    {
        auto &staging = sc->data->staging;

        std::lock_guard<std::mutex> lg(staging.updates.mtx);
        static_cast<void>(lg);

        pqxx::work txn(*conn_);
        pqxx::params p1;
        p1.append(deviceId);

        pqxx::result dres = txn.exec(
            "SELECT id FROM devices WHERE id = $1 LIMIT 1",
            p1);

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

        ///@todo убрать
        std::cout << "SIZE OF BEFORE " <<  staging.updates.devToReleaseMap.size() << std::endl;

        releaseId = updateInfoIt->second.releaseId;
        staging.updates.devToReleaseMap.erase(updateInfoIt);

        std::cout << "SIZE OF AFTER " <<  staging.updates.devToReleaseMap.size() << std::endl;;
    }

    pqxx::work txn(*conn_);

    {
        pqxx::params params;
        params.append(deviceId);
        params.append(releaseId);
        params.append(status);

        params.append(report.dump());

        pqxx::result r = txn.exec(INSERT_REPORT_SQL, params);
    }

    {
        pqxx::params params;
        params.append(deviceId);
        params.append(releaseId);
        params.append(status == 0 ? "success" : "failed");

        pqxx::result r = txn.exec(UPDATE_ASSIGMENT_STATUS_SQL, params);
    }

    txn.commit();
}
