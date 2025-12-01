#include "manifestservice.h"
#include "database.h"

json ManifestService::getManifest(uint32_t devId, const std::string &devType,
                                  const std::string &platform,
                                  const std::string &arch)
{
    pqxx::params prms;
    prms.append(devType);
    prms.append(platform);
    prms.append(arch);

    pqxx::work txn(*Database::instance().connection());
    pqxx::result res = txn.exec(
        "SELECT manifest_raw, signature_raw, active FROM releases "
        "WHERE device_type = $1 "
        "AND platform = $2 "
        "AND arch = $3 ",
        prms
    );

    if (res.empty())
    {
        throw std::runtime_error("No manifest found");
    }

    const pqxx::row& row  = res[0];

    std::string manifest  = row["manifest_raw"] .as<std::string>();
    std::string signature = row["signature_raw"].as<std::string>();
    bool        isActive  = row["active"]       .as<bool>();

    return json{{ "manifest",  manifest  },
                { "signature", signature }};

}
