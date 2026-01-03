#ifndef INTEGRATIONBASEFIXTURE_H
#define INTEGRATIONBASEFIXTURE_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <httplib.h>
#include <thread>
#include <filesystem>
#include <fstream>

#include "core/database.h"
#include "core/servercontext.h"
#include "mocks/archivetools_mock.h"
#include "mocks/cryptoutils_mock.h"


namespace fs = std::filesystem;
using namespace testing;

template<typename ControllerT>
class IntegrationBaseFixture : public ::testing::Test
{
protected:
    std::shared_ptr<ServerContext> sc;
    std::unique_ptr<ControllerT> controller;

    httplib::Server server;
    std::thread serverThread;
    std::unique_ptr<httplib::Client> client;
    const fs::path BASE_TEST_DIR = fs::temp_directory_path() / "pco_test";

    const std::string testDbname = "pco_test";

    void SetUp() override
    {
        Database::instance(testDbname, "postgres", "127.0.0.1", "5433");
        clearDatabase();

        auto crypto  = std::make_unique<NiceMock<MockCryptoUtils>>();
        auto archive = std::make_unique<NiceMock<MockArchiveTools>>();

        ON_CALL(*archive, extract(_, _, _))
            .WillByDefault([](const uint8_t*, size_t, const char* outdir) {
            fs::create_directories(outdir);
            std::ofstream(fs::path(outdir) / "dummy.bin") << "payload";
            return 0;
        });

        sc = std::make_shared<ServerContext>(std::move(crypto), std::move(archive));
    }

    void startServer(int port)
    {
        controller->registerRoute(server);
        serverThread = std::thread([this, port]() {
            server.listen("127.0.0.1", port);
        });

        while (!server.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        client = std::make_unique<httplib::Client>("127.0.0.1", port);
    }

    void clearDatabase()
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        txn.exec("TRUNCATE releases, devices, release_assignments, reports RESTART IDENTITY CASCADE;");
        txn.commit();
    }

    void TearDown() override
    {
        server.stop();
        if (serverThread.joinable())
            serverThread.join();

        clearDatabase();
        fs::remove_all(BASE_TEST_DIR);
    }
};
#endif // INTEGRATIONBASEFIXTURE_H
