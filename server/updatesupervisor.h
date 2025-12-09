#ifndef UPDATESUPERVISOR_H
#define UPDATESUPERVISOR_H

#include "database.h"
#include "servercontext.h"
#include <thread>
#include <atomic>


///@todo сделать базовый класс-поток
class UpdateSupervisor
{
public:
    UpdateSupervisor(ServerContext* sc, std::unique_ptr<pqxx::connection>&& conn);
    ~UpdateSupervisor();

private:
    void loop();
    ServerContext* sc_;
    std::thread suprevisorThread_;
    std::atomic<bool> stopFlag_;

    void processUpdate(const std::pair<uint64_t, UpdateInfo> &update);
    std::unique_ptr<pqxx::connection> conn_;
};

#endif // UPDATESUPERVISOR_H
