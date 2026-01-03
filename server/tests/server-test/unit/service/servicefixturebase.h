#ifndef SERVICEFIXTUREBASE_H
#define SERVICEFIXTUREBASE_H

#include <gtest/gtest.h>
#include <pqxx/pqxx>

#include <core/servercontext.h>

class ServiceFixtureBase : public ::testing::Test
{
protected:
    const std::string testDbname = "pco_test";
    const std::string connString = "dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433";

    std::unique_ptr<pqxx::connection> conn;
    std::shared_ptr<ServerContext> sc;

    void clearTables()
    {
        if (!conn || !conn->is_open())
        {
            try {
                conn = std::make_unique<pqxx::connection>(connString);
            } catch (const std::exception &e) {
                FAIL() << "Could not connect to test database: " << e.what();
            }
        }

        pqxx::work txn(*conn);
        txn.exec("TRUNCATE release_assignments, reports, devices, releases RESTART IDENTITY CASCADE;");
        txn.commit();
    }
};

#endif // SERVICEFIXTUREBASE_H
