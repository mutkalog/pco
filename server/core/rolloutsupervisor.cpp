#include "rolloutsupervisor.h"
#include <cmath>
#include <iostream>

namespace {

constexpr std::string_view ROLLOUT_CHECK_SQL =
    "SELECT\n"
    "    BOOL_OR(ra.status = 'failed') AS has_failed,\n"
    "    BOOL_OR ((now() - d.last_seen) > (d.poling_interval * 3 * interval '1 minute')) as inactive_device,\n"
    "    CEIL(COUNT(*) FILTER (WHERE ra.status = 'success') * 100.0 / COUNT(*)) AS success_percent,\n"
    "    r.canary_percent\n"
    "FROM release_assignments ra\n"
    "JOIN releases r ON ra.release_id = r.id\n"
    "JOIN devices d ON ra.device_id = d.id\n"
    "WHERE ra.release_id = $1\n"
    "GROUP BY r.canary_percent;\n";

constexpr std::string_view INVALIDATE_RELEASE_SQL =
    "UPDATE releases "
    "SET active = false,"
    "    is_canary = false "
    "WHERE id = $1 ";

constexpr std::string_view DEVICES_ASSIGNMENTS_SQL =
    " WITH total AS (\n"
    "    SELECT COUNT(*) AS cnt\n"
    "    FROM devices\n"
    "    WHERE device_type = $2\n"
    "      AND platform    = $3\n"
    "      AND arch        = $4\n"
    "      AND (now() - last_seen) < (poling_interval * 3 * interval '1 minute')\n"
    "),\n"
    "available AS (\n"
    "    SELECT id\n"
    "    FROM devices d\n"
    "    WHERE d.device_type = $2\n"
    "      AND d.platform    = $3\n"
    "      AND d.arch        = $4\n"
    "      AND (now() - last_seen) < (poling_interval * 3 * interval '1 minute')\n"
    "      AND NOT EXISTS (\n"
    "          SELECT 1\n"
    "          FROM release_assignments ra\n"
    "          WHERE ra.release_id = $1\n"
    "            AND ra.device_id = d.id\n"
    "      )\n"
    "    ORDER BY random()\n"
    "    LIMIT CEIL((SELECT cnt FROM total) * $5 / 100.0)\n"
    ")\n"
    "INSERT INTO release_assignments (release_id, device_id)\n"
    "SELECT $1, id\n"
    "FROM available;\n";

constexpr std::string_view UPDATE_CANARY_SQL =
    "UPDATE releases "
    "SET canary_percent = $2 "
    "WHERE id = $1 "
    "RETURNING canary_percent";

constexpr std::string_view COMMIT_CANNARY_SQL =
    "UPDATE releases "
    "SET is_canary = false, "
    "    canary_percent = 100, "
    "    active = true "
    "WHERE id = $1 ; ";

constexpr std::string_view REMOVE_ASSIGNMENTS_SQL =
    "DELETE FROM release_assignments "
    "WHERE release_id = $1 ";

constexpr std::string_view SET_RELEASES_INACTIVE_SQL =
    "UPDATE releases "
    "SET active = false "
    "WHERE device_type = $2 "
    "  AND platform = $3 "
    "  AND arch = $4 "
    "  AND id <> $1;";
}


RolloutSupervisor::RolloutSupervisor(std::shared_ptr<ServerContext> &sc)
    : Task("RolloutSupervisor", 10)
    , sc_(sc)
{}

void RolloutSupervisor::process()
{
    auto conn = cp_.acquire();
    try
    {
        auto& rollouts = sc_->staging.rollouts;

        std::unique_lock<std::mutex> ul(rollouts.mtx);
        rollouts.cv.wait(ul, [&](){
            std::cout << "RolloutSupervisor: checking" << std::endl;
            return rollouts.releaseToInfoMap.empty() == false;
        });

        std::vector<entry_id_t> idsToDelete;

        for (auto it = rollouts.releaseToInfoMap.begin();
             it != rollouts.releaseToInfoMap.end();
             ++it)
        {
            entry_id_t   id = it->first;
            RolloutInfo& ri = it->second;

            if (ri.isCanary == false)
            {
                std::cout << "RolloutSupervisor: got not canary release "
                          << it->first << std::endl;
                setReleasesInactive(*conn, *it);
                idsToDelete.push_back(it->first);
                continue;
            }

            pqxx::result res = checkRollout(*conn, id);

            if (res.empty())
            {
                assignDevices(*conn, *it);
                continue;
            }

            const pqxx::row& row  = res[0];

            bool updateFailed    = row["has_failed"]     .as<bool>();
            bool inactiveDevice  = row["inactive_device"].as<bool>();
            int  successPercent  = row["success_percent"].as<int>();
            int  inCanaryPercent = row["canary_percent"] .as<int>();

             std::cout << " percent: " << successPercent << ", goal: " << inCanaryPercent
                      << ", has failed: " << updateFailed
                      << ", inactive dev: " << inactiveDevice
                      << std::endl;

            if (updateFailed == true || inactiveDevice == true)
            {
                std::cout << "RolloutSupervisor: update failed"
                          << (inactiveDevice == true
                                  ? " due to inactive device in selection"
                                  : "")
                          << std::endl;
                invalidateRelease(*conn, id);
                removeAssignments(*conn, id);
                idsToDelete.push_back(it->first);
            }

            if (successPercent >= 100)
            {
                if (inCanaryPercent == 100)
                {
                    setReleasesInactive(*conn, *it);
                    commitCanary(*conn, id);
                    removeAssignments(*conn, id);
                    idsToDelete.push_back(it->first);
                }
                else
                {
                    int nextTarget             = std::clamp(std::ceil(ri.inRolloutPercentage * 2), 0.0, 100.0);
                    ri.nextSelectionPercentage = nextTarget - ri.inRolloutPercentage;
                    ri.inRolloutPercentage     = nextTarget;

                    updateCanary(*conn, *it);
                    assignDevices(*conn, *it);
                }
            }
        }

        for (const auto &id : idsToDelete)
        {
            rollouts.releaseToInfoMap.erase(id);
        }

        cp_.release(std::move(conn));
    }
    catch (...)
    {
        cp_.release(std::move(conn));
    }
}


pqxx::result RolloutSupervisor::checkRollout(pqxx::connection& conn, entry_id_t id)
{
    pqxx::work txn(conn);
    pqxx::params params;
    params.append(id);

    return txn.exec(ROLLOUT_CHECK_SQL, params);
}

void RolloutSupervisor::invalidateRelease(pqxx::connection &conn, entry_id_t id)
{
    pqxx::work txn(conn);
    pqxx::params params;
    params.append(id);

    txn.exec(INVALIDATE_RELEASE_SQL, params);
    txn.commit();
}

void RolloutSupervisor::assignDevices(pqxx::connection &conn, const std::pair<entry_id_t, RolloutInfo>& info)
{
    const auto& [id, ri] = info;
    std::cout << "RolloutManager: assigning " << ri.nextSelectionPercentage << "% more of "
              << ri.arch << " " << ri.type << " on " << ri.platform << " to "
              << id << " release" << std::endl;

    pqxx::work txn(conn);
    pqxx::params params;
    params.append(id);
    params.append(ri.type);
    params.append(ri.platform);
    params.append(ri.arch);
    params.append(ri.nextSelectionPercentage);

    pqxx::result res = txn.exec(DEVICES_ASSIGNMENTS_SQL, params);
    txn.commit();
}

void RolloutSupervisor::updateCanary(pqxx::connection &conn, const std::pair<entry_id_t, RolloutInfo>& info)
{
    const auto& [id, ri] = info;
    std::cout << "RolloutManager: updating percentage in "
              << id << " to "
              << ri.inRolloutPercentage << "%" << std::endl;

    pqxx::work txn(conn);
    pqxx::params params;
    params.append(id);
    params.append(ri.inRolloutPercentage);

    pqxx::result res = txn.exec(UPDATE_CANARY_SQL, params);
    txn.commit();
}

void RolloutSupervisor::commitCanary(pqxx::connection &conn, entry_id_t id)
{
    pqxx::work txn(conn);
    pqxx::params params;
    params.append(id);

    pqxx::result res = txn.exec(COMMIT_CANNARY_SQL, params);
    txn.commit();
}

void RolloutSupervisor::removeAssignments(pqxx::connection &conn, entry_id_t id)
{
    pqxx::work txn(conn);
    pqxx::params params;
    params.append(id);

    pqxx::result res = txn.exec(REMOVE_ASSIGNMENTS_SQL, params);
    txn.commit();

    std::cout << "RolloutManager: removed " << res.affected_rows()
              << " rows from release_assignment table" << std::endl;
}

void RolloutSupervisor::setReleasesInactive(pqxx::connection &conn, const std::pair<entry_id_t, RolloutInfo> &info)
{
    const auto& [id, ri] = info;
    pqxx::work txn(conn);
    pqxx::params params;
    params.append(id);
    params.append(ri.type);
    params.append(ri.platform);
    params.append(ri.arch);

    txn.exec(SET_RELEASES_INACTIVE_SQL, params);
    txn.commit();
}
