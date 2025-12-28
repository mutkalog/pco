#include <gtest/gtest.h>
#include <thread>
#include "core/connectionspull.h"


namespace {
const std::string testDbname = "pco_test";
}


TEST(ConnectionsPoolTest, AcquireReducesPoolSize)
{
    Database::instance(testDbname, "postgres", "127.0.0.1", "5433");
    ConnectionsPool pool(2);

    auto c1 = pool.acquire();
    ASSERT_TRUE(c1);
    ASSERT_TRUE(c1->is_open());

    ASSERT_EQ(pool.size(), 1);
}


TEST(ConnectionsPoolTest, ReleaseReturnsConnectionToPool)
{
    Database::instance(testDbname, "postgres", "127.0.0.1", "5433");
    ConnectionsPool pool(1);

    auto conn = pool.acquire();
    ASSERT_EQ(pool.size(), 0);

    pool.release(std::move(conn));

    ASSERT_EQ(pool.size(), 1);
}


TEST(ConnectionsPoolTest, AcquireBlocksUntilRelease)
{
    Database::instance(testDbname, "postgres", "127.0.0.1", "5433");
    ConnectionsPool pool(1);

    auto c1 = pool.acquire();
    ASSERT_EQ(pool.size(), 0);

    std::atomic<bool> acquired{false};

    std::thread t([&] {
        auto c2 = pool.acquire();
        acquired = true;
        pool.release(std::move(c2));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_FALSE(acquired);

    pool.release(std::move(c1));

    t.join();
    ASSERT_TRUE(acquired);
}


TEST(ConnectionsPoolTest, ClosedConnectionIsRecreatedOnAcquire)
{
    Database::instance(testDbname, "postgres", "127.0.0.1", "5433");
    ConnectionsPool pool(1);

    auto conn = pool.acquire();
    ASSERT_TRUE(conn->is_open());

    conn->close();
    ASSERT_FALSE(conn->is_open());

    pool.release(std::move(conn));

    auto newConn = pool.acquire();
    ASSERT_TRUE(newConn);
    ASSERT_TRUE(newConn->is_open());
}
