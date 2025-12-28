#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>


class Database
{
public:
    static Database &instance(const std::string &dbname = "pco",
                              const std::string &username = "postgres",
                              const std::string &host = "127.0.0.1",
                              const std::string &port = "5432");

    std::unique_ptr<pqxx::connection> getConnection();

private:
    Database(const std::string& dbname, const std::string& username, const std::string& host, const std::string& port);
    void ensureTriggerExists(pqxx::connection* conn);

    const std::string dbname_;
    const std::string username_;
    const std::string host_;
    const std::string port_;
};

inline Database &Database::instance(const std::string &dbname, const std::string &username, const std::string &host, const std::string &port)
{
    static Database inst(dbname, username, host, port);
    return inst;
}


#endif // DATABASE_H
