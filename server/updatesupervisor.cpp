#include "updatesupervisor.h"
#include "database.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

namespace {
const std::string MESSAGE    = "Server marked that update as failed due to timeout";
const int         ERROR_CODE = 3;

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

constexpr std::string_view UPDATE_DEV_INSTALLED_RELEASE_ID_SQL =
    "UPDATE devices \n"
    "SET release_id = $1\n"
    "WHERE id = $2\n";
}

UpdateSupervisor::UpdateSupervisor(std::shared_ptr<ServerContext> &sc)
    : Task("UpdateSupervisor", 10)
    , sc_(sc)
{
}


void UpdateSupervisor::process()
{
    auto& updates = sc_->staging.updates;
    {
        std::unique_lock<std::mutex> ul(updates.mtx);
        updates.cv.wait(ul, [&](){
            std::cout << "UpdateSupervisor: checking updates" << std::endl;
            return updates.devToReleaseMap.empty() == false;
        });

        std::vector<entry_id_t> idsToDelete;

        for (auto it = updates.devToReleaseMap.begin();
             it != updates.devToReleaseMap.end(); ++it)
        {
            if (std::chrono::steady_clock::now() >= it->second.expireTime
                || it->second.finished == true)
            {
                processUpdate(*it);
                idsToDelete.push_back(it->first);
            }
        }

        for (const auto& id : idsToDelete)
        {
            updates.devToReleaseMap.erase(id);

            std::cout << "UpdateSupervisor: size of updates in process map "
                      << updates.devToReleaseMap.size() << std::endl;
        }
    }
}

void UpdateSupervisor::processUpdate(const std::pair<uint64_t, UpdateInfo>& update)
{
    const auto& [devId, info] = update;

    bool timeout = info.finished == false;
    std::cout << "UpdateSupervisor: processing device " << devId << " report: ";
    std::cout << (timeout ? "device hung up" : "device gracefully reported" ) << std::endl;

    auto conn = cp_.acquire();

    pqxx::work txn(*conn);

    {
        pqxx::params params;
        params.append(devId);
        params.append(info.releaseId);
        params.append(timeout == true
                          ? ERROR_CODE
                          : info.status);
        params.append(timeout == true
                          ? json{"message", MESSAGE}.dump()
                          : info.report);

        pqxx::result r = txn.exec(INSERT_REPORT_SQL, params);
    }

    {
        pqxx::params params;
        params.append(update.first);
        params.append(update.second.releaseId);
        params.append(timeout == true
                             ? "failed"
                             : info.status == 0
                                  ? "success"
                                  : "failed");

        pqxx::result r = txn.exec(UPDATE_ASSIGMENT_STATUS_SQL, params);
    }

    if (timeout == false)
    {
        pqxx::params params;
        params.append(info.releaseId);
        params.append(devId);

        pqxx::result r = txn.exec(UPDATE_DEV_INSTALLED_RELEASE_ID_SQL, params);
    }

    txn.commit();

    cp_.release(std::move(conn));
}
