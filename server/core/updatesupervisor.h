#ifndef UPDATESUPERVISOR_H
#define UPDATESUPERVISOR_H

#include "core/connectionspull.h"
#include "core/servercontext.h"

#include "task.h"


class UpdateSupervisor final : public Task
{
    friend class UpdateSupervisorTest;

public:
    UpdateSupervisor(std::shared_ptr<ServerContext>& sc);

private:
    virtual void process() override;

    void processUpdate(const std::pair<uint64_t, UpdateInfo> &update);

    std::shared_ptr<ServerContext> sc_;
    ConnectionsPool cp_;
};

#endif // UPDATESUPERVISOR_H
