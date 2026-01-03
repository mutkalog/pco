#ifndef ROLLOUTSUPERVISOR_H
#define ROLLOUTSUPERVISOR_H

#include "core/servercontext.h"
#include "core/connectionspull.h"

#include "task.h"


class RolloutSupervisor final : public Task
{
    friend class RolloutSupervisorTest;
    friend class UploadIntegrationTest;

public:
    RolloutSupervisor(std::shared_ptr<ServerContext>& sc);

private:
    virtual void process() override;

    pqxx::result checkRollout(pqxx::connection& conn, entry_id_t id);
    void invalidateRelease(pqxx::connection& conn, entry_id_t id);
    void assignDevices(pqxx::connection& conn, const std::pair<entry_id_t, RolloutInfo> &info);
    void updateCanary(pqxx::connection& conn, const std::pair<entry_id_t, RolloutInfo> &info);
    void commitCanary(pqxx::connection& conn, entry_id_t id);
    void removeAssignments(pqxx::connection& conn, entry_id_t id);
    void setReleasesInactive(pqxx::connection& conn, const std::pair<entry_id_t, RolloutInfo> &info);

    std::shared_ptr<ServerContext> sc_;
    ConnectionsPool cp_;
};

#endif // ROLLOUTSUPERVISOR_H
