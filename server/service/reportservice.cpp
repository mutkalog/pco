#include "reportservice.h"
#include "database.h"


void ReportService::parseReport(const json &report)
{
    std::string type     = report["type"];
    std::string arch     = report["arch"];
    std::string platform = report["platform"];
    uint64_t    deviceId = report["id"];
    std::string version  = report["current_version"];
    int         status   = report["error"]["code"];

    pqxx::work txn(*Database::instance().connection());
    pqxx::params p1;
    p1.append(deviceId);

    pqxx::result dres = txn.exec(
        "SELECT id FROM devices WHERE id = $1 LIMIT 1",
        p1);

    if (dres.empty() == true || dres[0][0].as<uint64_t>() != deviceId)
        throw std::runtime_error("Cannot find such device id");

    pqxx::params params;
    params.append(type);
    params.append(platform);
    params.append(arch);
    params.append(version);
    params.append(deviceId);
    params.append(status);
    params.append(report.dump());

    pqxx::result r = txn.exec(
            "INSERT INTO reports (device_id, release_id, status, body) "
            "VALUES ( "
            "    $5, "
            "    (SELECT id FROM releases "
            "     WHERE device_type = $1 "
            "       AND platform = $2 "
            "       AND arch = $3 "
            "       AND version = $4), "
            "    $6, "
            "    $7::jsonb "
            ") "
            "RETURNING id;",
            params);

    txn.commit();
}
