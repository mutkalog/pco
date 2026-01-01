#include "integrationfixturebase.h"
#include <fstream>

class RegistrationExecutorIntegrationTest : public IntegrationFixtureBase
{
protected:
    json originalConfig;
    std::atomic<int> nextDeviceId{100};

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();
        backupConfig();

        server->Post("/register", [this](const httplib::Request& req, httplib::Response& res) {
            json response = {
                {"status", "registered"},
                {"id", nextDeviceId++}
            };
            res.set_content(response.dump(), "application/json");
            res.status = 200;
        });

        startServer();
    }

    void TearDown() override
    {
        restoreConfig();
        IntegrationFixtureBase::TearDown();
    }

    void backupConfig()
    {
        std::ifstream file(CONF_FILE);
        if (file.is_open())
        {
            file >> originalConfig;
        }
    }

    void restoreConfig()
    {
        std::ofstream file(CONF_FILE);
        if (file.is_open())
        {
            file << originalConfig.dump(4);
        }
    }

    json readConfig()
    {
        std::ifstream file(CONF_FILE);
        json config;
        if (file.is_open())
        {
            file >> config;
        }
        return config;
    }

    void writeConfig(const json& config)
    {
        std::ofstream file(CONF_FILE);
        if (file.is_open())
        {
            file << config.dump(4);
        }
    }

    void removeConfigField(const std::string& field)
    {
        json config = readConfig();
        config.erase(field);
        writeConfig(config);
    }

    int getDeviceIdFromConfig()
    {
        json config = readConfig();
        return config.value("id", 0);
    }

    bool configHasId()
    {
        json config = readConfig();
        return config.contains("id");
    }
};


TEST_F(RegistrationExecutorIntegrationTest, UnregisteredDeviceSuccessfullyRegistersAndTransitsToIdle)
{
    createDeviceConfig(CONF_FILE, 123);

    removeConfigField("id");
    ASSERT_FALSE(configHasId()) << "Precondition: id must be removed from config";

    createStateMachine("", StateExecutor::REGISTRATION);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
    EXPECT_TRUE(configHasId()) << "Device should receive id after registration";
    EXPECT_GT(getDeviceIdFromConfig(), 0) << "Registered id must be positive";
}


TEST_F(RegistrationExecutorIntegrationTest, AlreadyRegisteredDeviceSkipsRegistrationAndTransitsToIdle)
{
    createDeviceConfig(CONF_FILE, 123);

    ASSERT_TRUE(configHasId()) << "Precondition: config must have id";
    int originalId = getDeviceIdFromConfig();
    ASSERT_GT(originalId, 0);

    createStateMachine("", StateExecutor::REGISTRATION);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
    EXPECT_EQ(getDeviceIdFromConfig(), originalId) << "Id should remain unchanged";
}


TEST_F(RegistrationExecutorIntegrationTest, RegisteredIdPersistsAcrossRestarts)
{
    createDeviceConfig(CONF_FILE, 123);

    removeConfigField("id");
    createStateMachine("", StateExecutor::REGISTRATION);
    stateMachine->run();

    int registeredId = getDeviceIdFromConfig();
    ASSERT_GT(registeredId, 0);

    stateMachine.reset();
    createStateMachine("", StateExecutor::REGISTRATION);

    EXPECT_EQ(stateMachine->context.devconf->id(), registeredId);

    stateMachine->run();
    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
}
