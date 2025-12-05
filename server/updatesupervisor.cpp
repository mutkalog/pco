#include "updatesupervisor.h"
#include "database.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

namespace {
const std::string MESSAGE    = "Server marked that update as failed due to timeout";
const int         ERROR_CODE = 3;

///@todo придумать общее хранилище для sql
/// такие же sql есть в reportservice.cpp
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

UpdateSupervisor::UpdateSupervisor(ServerContext* sc, std::unique_ptr<pqxx::connection>&& conn)
    : suprevisorThread_(std::thread(&UpdateSupervisor::loop, this))
    , stopFlag_(false)
    , conn_(Database::instance().getConnection())
    , sc_(sc)
{
}

UpdateSupervisor::~UpdateSupervisor()
{
    stopFlag_ = true;
    suprevisorThread_.join();
}

void UpdateSupervisor::loop()
{
    while (stopFlag_ == false)
    {
        {
            auto& staging = sc_->data->staging;

            std::lock_guard<std::mutex> lg(staging.updates.mtx);
            static_cast<void>(lg);

            std::vector<decltype(staging.updates.devToReleaseMap.begin())> iteratorsToDelete;

            for (auto it = staging.updates.devToReleaseMap.begin();
                 it != staging.updates.devToReleaseMap.end(); ++it)
            {
                if (std::chrono::steady_clock::now() >= it->second.expireTime)
                {
                    iteratorsToDelete.push_back(it);
                }
            }

            for (const auto& it : iteratorsToDelete)
            {
                markFailed(*it);
                staging.updates.devToReleaseMap.erase(it);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void UpdateSupervisor::markFailed(std::pair<uint64_t, UpdateInfo> update)
{
    std::cout << "UpdateSupervisor:: marking device "
              << update.first << " with releaseId "
              << update.second.releaseId << " failed" << std::endl;

    pqxx::params params;
    params.append(update.first);
    params.append(update.second.releaseId);
    params.append(ERROR_CODE);
    params.append(json{"message", MESSAGE}.dump());

    // pqxx::work txn(*conn_);

    // pqxx::result r = txn.exec(
    //         "INSERT INTO reports (device_id, release_id, status, body) "
    //         "VALUES ( "
    //         "    $1, "
    //         "    $2, "
    //         "    $3, "
    //         "    $4::jsonb "
    //         ") "
    //         "RETURNING id;",
            // params);


    pqxx::work txn(*conn_);

    {
        pqxx::params params;
        params.append(update.first);
        params.append(update.second.releaseId);
        params.append(ERROR_CODE);
        params.append(json{"message", MESSAGE}.dump());

        pqxx::result r = txn.exec(INSERT_REPORT_SQL, params);
    }

    {
        pqxx::params params;
        params.append(update.first);
        params.append(update.second.releaseId);
        params.append("failed");

        pqxx::result r = txn.exec(UPDATE_ASSIGMENT_STATUS_SQL, params);
    }


    txn.commit();
}
