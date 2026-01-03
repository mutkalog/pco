#include <gtest/gtest.h>
#include "core/database.h"

namespace {
const std::string testDbname = "pco_test";
}


class DatabaseTest : public ::testing::Test
{
protected:
    std::unique_ptr<pqxx::connection> conn;

    void SetUp() override
    {
        auto db = Database::instance(testDbname, "postgres", "127.0.0.1", "5433");
        static_cast<void>(db);

        conn = std::make_unique<pqxx::connection>
            ("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
    }

    void TearDown() override
    {
        conn->close();
    }
};


TEST_F(DatabaseTest, TablesAreCreated)
{
    pqxx::work txn(*conn);
    pqxx::result r;

    r = txn.exec(
        "SELECT to_regclass('public.releases')"
    );
    ASSERT_EQ(r[0][0].c_str(), std::string("releases"));

    r = txn.exec(
        "SELECT to_regclass('public.devices')"
    );
    ASSERT_EQ(r[0][0].c_str(), std::string("devices"));

    r = txn.exec(
        "SELECT to_regclass('public.release_assignments')"
    );
    ASSERT_EQ(r[0][0].c_str(), std::string("release_assignments"));

    r = txn.exec(
        "SELECT to_regclass('public.reports')"
    );
    ASSERT_EQ(r[0][0].c_str(), std::string("reports"));

    r = txn.exec(
        "SELECT to_regclass('public.releases_unique_idx')"
    );
    ASSERT_EQ(r[0][0].c_str(), std::string("releases_unique_idx"));
}


TEST_F(DatabaseTest, TriggerIsCreated)
{
    pqxx::work txn(*conn);
    pqxx::result r;

    r = txn.exec(
        "SELECT tgname FROM pg_trigger WHERE tgname='trg_reports_status';"
    );
    ASSERT_EQ(r.size(), 1);
    ASSERT_EQ(r[0][0].c_str(), std::string("trg_reports_status"));
}


TEST_F(DatabaseTest, GetConnectionReturnsOpenConnection)
{
    auto conn = Database::instance().getConnection();
    ASSERT_TRUE(conn->is_open());
}
