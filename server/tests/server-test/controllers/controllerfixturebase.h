#ifndef CONTROLLERFIXTUREBASE_H
#define CONTROLLERFIXTUREBASE_H

#include <core/servercontext.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <httplib.h>
#include <thread>
#include "mocks/archivetools_mock.h"
#include "mocks/cryptoutils_mock.h"

using ::testing::NiceMock;

template<typename ControllerT, typename MockServiceT>
class ControllerFixtureBase : public ::testing::Test
{
protected:
    std::shared_ptr<ServerContext> sc;
    std::unique_ptr<MockServiceT> service;
    MockServiceT* servicePtr;
    std::unique_ptr<ControllerT> controller;
    httplib::Server server;
    std::thread serverThread;
    std::unique_ptr<httplib::Client> client;

    void SetUp() override
    {
        setupContext();
        service    = std::make_unique<NiceMock<MockServiceT>>();
        servicePtr = service.get();
    }

    virtual void setupContext()
    {
        auto crypto  = std::make_unique<NiceMock<MockCryptoUtils>>();
        auto archive = std::make_unique<NiceMock<MockArchiveTools>>();

        sc = std::make_shared<ServerContext>(
            std::move(crypto), std::move(archive)
        );
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

    void TearDown() override
    {
        server.stop();
        if (serverThread.joinable())
            serverThread.join();
    }
};
#endif // CONTROLLERFIXTUREBASE_H
