#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>


class Database
{
public:
    auto connection() { return conn_; }
    static Database& instance();

private:
    Database();
    void ensureTriggerExists();
    std::shared_ptr<pqxx::connection> conn_;
};

inline Database &Database::instance()
{
    static Database inst;
    return inst;
}

#endif // DATABASE_H
