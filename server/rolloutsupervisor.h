#ifndef ROLLOUTSUPERVISOR_H
#define ROLLOUTSUPERVISOR_H

#include "servercontext.h"
#include "database.h"

#include <atomic>
#include <thread>


///@todo проверять is_canary при запуске
class RolloutSupervisor
{
public:
    RolloutSupervisor(ServerContext *sc, std::unique_ptr<pqxx::connection> &&conn);
    ~RolloutSupervisor();

private:
    void loop();
    std::thread suprevisorThread_;
    std::atomic<bool> stopFlag_;
    pqxx::result checkRollout(entry_id_t id);
    void invalidateRelease(entry_id_t id);
    void assignDevices(const std::pair<entry_id_t, RolloutInfo> &info);
    void updateCanary(const std::pair<entry_id_t, RolloutInfo> &info);
    void commitCanary(entry_id_t id);
    void removeAssignments(entry_id_t id);
    void setReleasesInactive(const std::pair<entry_id_t, RolloutInfo> &info);

    ServerContext* sc_;
    std::unique_ptr<pqxx::connection> conn_;
};

#endif // ROLLOUTSUPERVISOR_H
