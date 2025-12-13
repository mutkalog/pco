#ifndef UPDATESUPERVISOR_H
#define UPDATESUPERVISOR_H

#include "connectionspull.h"
#include "servercontext.h"

#include "common/task.h"


class UpdateSupervisor final : public Task
{
public:
    UpdateSupervisor(std::shared_ptr<ServerContext>& sc);

private:
    virtual void process() override;

    void processUpdate(const std::pair<uint64_t, UpdateInfo> &update);

    std::shared_ptr<ServerContext> sc_;
    ConnectionsPool cp_;
};

#endif // UPDATESUPERVISOR_H
