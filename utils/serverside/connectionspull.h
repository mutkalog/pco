#ifndef CONNECTIONSPULL_H
#define CONNECTIONSPULL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "database.h"

class ConnectionsPool
{
public:
    ConnectionsPool(size_t poolSize = 10u);

    std::unique_ptr<pqxx::connection> acquire();
    void release(std::unique_ptr<pqxx::connection> conn);
    size_t size() { return pool_.size(); };

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::unique_ptr<pqxx::connection>> pool_;
};

inline
ConnectionsPool::ConnectionsPool(size_t poolSize)
{
    auto& db = Database::instance();

    for (size_t i = 0; i < poolSize; ++i)
    {
        pool_.emplace(db.getConnection());
    }
}

inline std::unique_ptr<pqxx::connection>
ConnectionsPool::acquire()
{
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]{ return pool_.empty() == false; });
    auto conn = std::move(pool_.front());
    if (conn->is_open() == false)
    {
        conn = Database::instance().getConnection();
    }
    pool_.pop();
    return conn;
}

inline void
ConnectionsPool::release(std::unique_ptr<pqxx::connection> conn)
{
    std::lock_guard<std::mutex> lock(mtx_);
    pool_.push(std::move(conn));
    cv_.notify_one();
}

#endif
