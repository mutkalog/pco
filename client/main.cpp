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


int main()
{
    try
    {
        const fs::path CONF_FILE             = "/var/pco/devconfig.json";
        const fs::path LAST_UPDATE_INFO_FILE = "/var/pco/last-update.json";
        const fs::path STATE_FILE            = "/var/pco/state.json";
        const fs::path STAGING_DIR           = "/var/pco/staging";

        auto devconf = std::make_unique<DeviceConfig>(CONF_FILE, LAST_UPDATE_INFO_FILE);
        devconf->loadConfig();
        devconf->loadPrevManifest();

        std::string clientCert = devconf->certPath().string();
        std::string clientKey  = devconf->keyPath().string();
        std::string caCert     = devconf->caCertPath().string();
        std::string host       = devconf->serverUrl();
        int         port       = devconf->serverPort();

        auto client       = std::make_unique<SslHttpClient>(host, port, clientCert, clientKey, caCert);
        auto cryptoUtils  = std::make_unique<SSLUtilsAdapter>();
        auto archiveTools = std::make_unique<ArchiveToolsAdapter>();
        auto systemCalls  = std::make_unique<SystemCalls>();
        UpdateContext context(std::move(devconf),
                              std::move(client),
                              std::move(cryptoUtils),
                              std::move(archiveTools),
                              std::move(systemCalls),
                              STAGING_DIR,
                              LAST_UPDATE_INFO_FILE);

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

        StateMachine stateMachine(std::move(statePersistence),
                                  std::move(context), std::move(idToStateMap),
                                  StateExecutor::REGISTRATION,
                                  StateExecutor::VERIFYING);
        stateMachine.init();

        while (true)
        {
            stateMachine.run();
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "UpdateContext: something went wrong."
                     " Aborting" << std::endl;
        std::cout << e.what()    << std::endl;
        exit(EXIT_FAILURE);
    }
}
