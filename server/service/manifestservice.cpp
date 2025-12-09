#include "manifestservice.h"
#include "database.h"
#include <iostream>

namespace {
const std::string CANNARY_SQL  =
    "SELECT r.manifest_raw, r.signature_raw, r.is_canary "
    "FROM release_assignments ra "
    "JOIN releases r ON r.id = ra.release_id "
    "WHERE ra.device_id = $1";

const std::string FALLBACK_SQL =
    "SELECT r.manifest_raw, r.signature_raw "
    "FROM releases r "
    "JOIN devices d ON d.id = $1 "
     "AND d.device_type = r.device_type "
     "AND d.platform = r.platform "
     "AND d.arch = r.arch "
    "WHERE r.device_type = $2 "
     "AND r.platform = $3 "
     "AND r.arch = $4 "
     "AND r.active = true "
     "AND r.is_canary = false";

constexpr std::string_view UPDATE_DEV_TS_SQL =
    "UPDATE devices "
    "SET last_seen = now() "
    "WHERE id = $1";
}


json ManifestService::getManifest(uint64_t devId, const std::string &devType,
                                  const std::string &platform,
                                  const std::string &arch)
{
    auto conn = cp_.acquire();
    try
    {
        std::cout << "Manifest DB connecions pool size: " << cp_.size() << std::endl;
        {
            pqxx::work txn(*conn);
            pqxx::params params;
            params.append(devId);
            txn.exec(UPDATE_DEV_TS_SQL, params);
            txn.commit();
        }

        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(devId);

        pqxx::result res = txn.exec(CANNARY_SQL, params);

        if (res.empty() == true)
        {
            params.append(devType);
            params.append(platform);
            params.append(arch);
            res = txn.exec(FALLBACK_SQL, params);

            if (res.empty() == true)
            {
                throw std::runtime_error("No manifest found");
            }
        }

        const pqxx::row& row  = res[0];

        std::string manifest  = row["manifest_raw"] .as<std::string>();
        std::string signature = row["signature_raw"].as<std::string>();

        json result{{ "manifest",  manifest  },
                    { "signature", signature }};

        cp_.release(std::move(conn));

        return result;
    }
    catch (...)
    {
        cp_.release(std::move(conn));
        throw;
    }
}
