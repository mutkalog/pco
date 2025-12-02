#include "manifestservice.h"
#include "database.h"

json ManifestService::getManifest(uint64_t devId, const std::string &devType,
                                  const std::string &platform,
                                  const std::string &arch)
{
    pqxx::params prms;
    prms.append(devId);
    prms.append(devType);
    prms.append(platform);
    prms.append(arch);

    pqxx::work txn(*Database::instance().connection());
    pqxx::result res = txn.exec(
        "SELECT r.manifest_raw, r.signature_raw, r.active "
        "FROM releases r "
        "JOIN devices d ON d.id = $1 "
            "AND d.device_type = r.device_type "
            "AND d.platform = r.platform "
            "AND d.arch = r.arch "
        "WHERE r.device_type = $2 "
            "AND r.platform = $3 "
            "AND r.arch = $4 "
            "AND r.active = true",
        prms
    );

    if (res.empty())
    {
        throw std::runtime_error("No manifest found");
    }
    else if (res.size() > 1)
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

