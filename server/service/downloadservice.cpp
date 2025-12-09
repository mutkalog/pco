#include "downloadservice.h"
#include "../../utils/common/archive.h"
#include "database.h"
#include <iostream>

namespace {
constexpr std::string_view CANNARY_SQL =
    "SELECT r.file_paths, r.id \n"
    "FROM release_assignments ra \n"
    "JOIN releases r on r.id = ra.release_id\n"
    "WHERE ra.device_id = $1;\n";

constexpr std::string_view FALLBACK_SQL =
    "SELECT r.file_paths, r.id "
    "FROM releases r "
    "JOIN devices d ON d.id = $1 "
        "AND d.device_type = r.device_type "
        "AND d.platform = r.platform "
        "AND d.arch = r.arch "
    "WHERE r.device_type = $2 "
        "AND r.platform = $3 "
        "AND r.arch = $4 "
        "AND r.is_canary = false "
        "AND r.active = true";

constexpr uint64_t ERROR_CODE = 3;
}

std::vector<uint8_t>
DownloadService::getArchive(ServerContext *sc, uint32_t devId,
                            const std::string &devType,
                            const std::string &platform,
                            const std::string &arch)
{
    auto passUpdateInfo = [&sc](uint64_t devId, uint64_t relId)
    {
        auto& updates = sc->data->staging.updates;
        std::lock_guard<std::mutex> lg(updates.mtx);
        static_cast<void>(lg);

        UpdateInfo info {
           relId,
            false,
            3,
            "",
           std::chrono::steady_clock::now() + sc->data->updateTimeoutSeconds
        };

        updates.devToReleaseMap.insert({devId, info});
        updates.cv.notify_all();
        std::cout << "Inserted to devToReleaseMap dev " << devId << std::endl;
    };

    auto conn = cp_.acquire();
    bool updateInfoPassed = false;
    try
    {
        std::cout << "Download DB connecions pool size: " << cp_.size() << std::endl;
        pqxx::result res;
        pqxx::work txn(*conn);
        pqxx::params prms;
        prms.append(devId);

        res = txn.exec(CANNARY_SQL, prms);

        if (res.empty() == true)
        {
            prms.append(devType);
            prms.append(platform);
            prms.append(arch);
            res = txn.exec(FALLBACK_SQL, prms);

            if (res.empty())
            {
                throw pqxx::sql_error("No manifest found");
            }
        }

        std::vector<std::string> paths{};

        const pqxx::row& row       = res[0];
        std::string      raw       = row["file_paths"].c_str();
        uint64_t         releaseId = row["id"].as<uint64_t>();

        passUpdateInfo(devId, releaseId);
        updateInfoPassed = true;

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

        cp_.release(std::move(conn));

        return archive;
    }
    catch (...)
    {
        if (updateInfoPassed == false)
        {
            std::cout << "UpdateInfo has not been passed to UpdateSupervisor. "
                         "Passing fail info"
                      << std::endl;
            passUpdateInfo(devId, -1);
        }

        cp_.release(std::move(conn));
        throw;
    }
}
