#include "downloadservice.h"
#include "../../utils/common/archive.h"
#include "database.h"
#include <iostream>


std::vector<uint8_t>
DownloadService::getArchive(ServerContext *sc, uint32_t devId,
                            const std::string &devType,
                            const std::string &platform,
                            const std::string &arch)
{
    pqxx::params prms;
    prms.append(devId);
    prms.append(devType);
    prms.append(platform);
    prms.append(arch);

    pqxx::work txn(*conn_);

    pqxx::result res = txn.exec(
        "SELECT r.file_paths, r.id "
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
        throw std::runtime_error("No release found");
    }
    else if (res.size() > 1)
    {
        throw std::system_error(std::error_code(0, std::generic_category()),
                                "Server error");
    }

    std::vector<std::string> paths{};

    const pqxx::row& row       = res[0];
    std::string      raw       = row["file_paths"].c_str();
    uint64_t         releaseId = row["id"].as<uint64_t>();

    pqxx::array_parser parser(raw);

    while (true)
    {
        auto [token, value] = parser.get_next();

        if (token == pqxx::array_parser::juncture::done)
            break;

        if (token == pqxx::array_parser::juncture::string_value)
            paths.emplace_back(value);
    }

    if (paths.empty() == true)
        throw std::system_error(std::error_code(0, std::generic_category()),
                                "Release has no files");


    std::vector<uint8_t> archive;
    if (create_archive_from_paths(paths, archive) != ARCHIVE_OK)
        throw std::runtime_error("Cannot compress binaries");

    {
        auto& staging = sc->data->staging;
        std::lock_guard<std::mutex> lg(staging.updates.mtx);
        static_cast<void>(lg);
        UpdateInfo info = {releaseId,
                           std::chrono::steady_clock::now() + sc->data->updateTimeoutSeconds};

        staging.updates.devToReleaseMap.insert({devId, info});
        std::cout << "data ptr in controller = " << sc->data.get() << std::endl;
    }

    return archive;
}
