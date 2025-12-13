#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>


class Database
{
public:
    static Database& instance();
    std::unique_ptr<pqxx::connection> getConnection();

private:
    Database();
    void ensureTriggerExists(pqxx::connection* conn);
};

inline Database &Database::instance()
{
    static Database inst;
    return inst;
}

#endif // DATABASE_H
