#ifndef INTEGRATIONFIXTUREBASE_H
#define INTEGRATIONFIXTUREBASE_H

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <fstream>

#include "core/sslhttpclient.h"
#include "core/statemachine.h"
#include "core/statepersistence.h"
#include "core/systemcalls.h"
#include "core/stateexecutors/stateexecutor.h"
#include "core/stateexecutors/checkingstateexecutor.h"
#include "core/stateexecutors/committingstateexecutor.h"
#include "core/stateexecutors/dowloadingstateexecutor.h"
#include "core/stateexecutors/finalizingstateexecutor.h"
#include "core/stateexecutors/installingstateexecutor.h"
#include "core/stateexecutors/preparingstateexecutor.h"
#include "core/stateexecutors/verifyingstateexecutor.h"
#include "core/stateexecutors/registrationexecutor.h"
#include "core/stateexecutors/idlestateexecutor.h"
#include "archivetoolsadapter.h"
#include "sslutilsadapter.h"


class FastSystemCalls : public SystemCalls
{
public:
    int sleep(unsigned int secs) override { return 0; }
};

class IntegrationFixtureBase : public testing::Test
{
protected:
    const fs::path BASE_TEST_DIR = fs::temp_directory_path() / "pco";
    const fs::path CONF_FILE     = BASE_TEST_DIR / "devconfig.json";
    const fs::path STATE_FILE    = BASE_TEST_DIR / "state.json";
    const fs::path STAGING_DIR   = BASE_TEST_DIR / "staging";

    const fs::path SERVER_CERT = fs::path(PROJECT_ROOT_DIR) / "client/tests/resources/security/server.crt";
    const fs::path SERVER_KEY  = fs::path(PROJECT_ROOT_DIR) / "client/tests/resources/security/server.key";
    const fs::path CA_CERT     = fs::path(PROJECT_ROOT_DIR) / "client/tests/resources/security/ca.crt";

    std::unique_ptr<StateMachine> stateMachine;
    std::unique_ptr<httplib::SSLServer> server;
    std::thread serverThread;
    const int serverPort = 18992;

    void SetUp() override
    {
        fs::create_directories(STAGING_DIR);
        fs::create_directories(STATE_FILE.parent_path());

        initServer();
    }

    void TearDown() override
    {
        stopServer();
        fs::remove_all(BASE_TEST_DIR);
    }

    void initServer()
    {
        server = std::make_unique<httplib::SSLServer>(
            SERVER_CERT.string().c_str()
            , SERVER_KEY.string().c_str()
        );

        if (!server->is_valid())
        {
            throw std::runtime_error("Failed to initialize SSL server");
        }
    }

    void startServer()
    {
        if (serverThread.joinable()) return;

        serverThread = std::thread([this]() {
            server->listen("0.0.0.0", serverPort);
            std::cout << "server done " << std::endl;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    void stopServer()
    {
        if (server)
        {
            server->stop();
        }
        if (serverThread.joinable())
        {
            serverThread.join();
        }
    }

    void createDeviceConfig(const fs::path& savePath, std::optional<int> deviceId)
    {
        fs::path rootDir = fs::path(PROJECT_ROOT_DIR);

        json config = {
            {"type", "Raspberry Pi 4"},
            {"platform", "Linux"},
            {"arch", "ARMv8"},
            {"updatePollingIntervalMinutes", 1},
            {"serverURL", "localhost"},
            {"serverPort", 18992},
            {"certPath",  (rootDir / "client/tests/resources/security/client.crt").string()},
            {"keyPath",   (rootDir / "client/tests/resources/security/client.key").string()},
            {"caCertPath",(rootDir / "client/tests/resources/security/ca.pem").string()},
            {"publicKeyPath", (rootDir / "client/tests/resources/security/public.pem").string()},
        };

        if (deviceId.has_value())
        {
            config["id"] = deviceId.value();
        }

        std::ofstream ofs(savePath);
        if (!ofs)
            throw std::runtime_error("Cannot open file for writing: " + savePath.string());

        ofs << config.dump(2);
        ofs.flush();
    }

    void createStateMachine(const fs::path& lastUpdateFile,
                            StateExecutor::StateId initialState = StateExecutor::REGISTRATION)
    {
        auto devconf = std::make_unique<DeviceConfig>(CONF_FILE, lastUpdateFile);
        devconf->loadConfig();
        devconf->loadPrevManifest();

        std::string clientCert = devconf->certPath().string();
        std::string clientKey  = devconf->keyPath().string();
        std::string caCert     = devconf->caCertPath().string();
        std::string host       = devconf->serverUrl();
        int         port       = serverPort;

        auto client       = std::make_unique<SslHttpClient>(host, port, clientCert, clientKey, caCert);
        auto cryptoUtils  = std::make_unique<SSLUtilsAdapter>();
        auto archiveTools = std::make_unique<ArchiveToolsAdapter>();
        auto systemCalls  = std::make_unique<FastSystemCalls>();

        UpdateContext context(std::move(devconf),
                              std::move(client),
                              std::move(cryptoUtils),
                              std::move(archiveTools),
                              std::move(systemCalls),
                              STAGING_DIR,
                              lastUpdateFile);

        auto statePersistence = std::make_unique<StatePersistence>(STATE_FILE);

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(StateExecutor::REGISTRATION, std::make_unique<RegistrationStateExecutor>(StateExecutor::REGISTRATION));
        idToStateMap.emplace(StateExecutor::IDLE,         std::make_unique<IdleStateExecutor>(StateExecutor::IDLE));
        idToStateMap.emplace(StateExecutor::CHECKING,     std::make_unique<CheckingStateExecutor>(StateExecutor::CHECKING));
        idToStateMap.emplace(StateExecutor::DOWNLOADING,  std::make_unique<DownloadingStateExecutor>(StateExecutor::DOWNLOADING));
        idToStateMap.emplace(StateExecutor::VERIFYING,    std::make_unique<VerifyingStateExecutor>(StateExecutor::VERIFYING));
        idToStateMap.emplace(StateExecutor::PREPARING,    std::make_unique<PreparingStateExecutor>(StateExecutor::PREPARING));
        idToStateMap.emplace(StateExecutor::INSTALLING,   std::make_unique<InstallingStateExecutor>(StateExecutor::INSTALLING));
        idToStateMap.emplace(StateExecutor::COMMITTING,   std::make_unique<CommittingStateExecutor>(StateExecutor::COMMITTING));
        idToStateMap.emplace(StateExecutor::FINALIZING,   std::make_unique<FinalizingStateExecutor>(StateExecutor::FINALIZING));

        stateMachine = std::make_unique<StateMachine>(
            std::move(statePersistence),
            std::move(context),
            std::move(idToStateMap),
            initialState,
            StateExecutor::VERIFYING);

        stateMachine->init();
    }

    void writeFile(const fs::path& path, const std::string& data)
    {
        fs::create_directories(path.parent_path());
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs)
            throw std::runtime_error("cannot open file for writing: " + path.string());
        ofs.write(data.data(), data.size());
    }

    std::string readFileBinary(const fs::path& path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs)
            throw std::runtime_error("cannot open file for reading: " + path.string());
        std::ostringstream ss;
        ss << ifs.rdbuf();
        return ss.str();
    }
};

#endif // INTEGRATIONFIXTUREBASE_H
