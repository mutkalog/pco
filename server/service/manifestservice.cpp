#include "manifestservice.h"
#include "database.h"

namespace {
const std::string CANNARY_SQL  = "SELECT r.manifest_raw, r.signature_raw, r.is_canary "
                                 "FROM release_assignments ra "
                                 "JOIN releases r ON r.id = ra.release_id "
                                 "WHERE ra.device_id = $1";

const std::string FALLBACK_SQL = "SELECT r.manifest_raw, r.signature_raw "
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
}

json ManifestService::getManifest(uint64_t devId, const std::string &devType,
                                  const std::string &platform,
                                  const std::string &arch)
{
    pqxx::work txn(*conn_);

    pqxx::params prms;
    prms.append(devId);

    pqxx::result res = txn.exec(CANNARY_SQL, prms);

    if (res.empty() == true)
    {
        prms.append(devType);
        prms.append(platform);
        prms.append(arch);
        res = txn.exec(FALLBACK_SQL, prms);

        if (res.empty())
        {
            throw std::runtime_error("No manifest found");
        }
    }

    if (res.size() > 1)
    {
        throw std::system_error(std::error_code(0, std::generic_category()),
                                "Server error");
    }

    const pqxx::row& row  = res[0];

    std::string manifest  = row["manifest_raw"] .as<std::string>();
    std::string signature = row["signature_raw"].as<std::string>();

    return json{{ "manifest",  manifest  },
                { "signature", signature }};
}
