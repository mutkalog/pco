#include "integrationfixturebase.h"

class FinalizingExecutorIntegrationTest : public IntegrationFixtureBase
{
protected:
    fs::path appPath;
    fs::path rbPath;
    fs::path lastUpdateFile;

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();

        appPath        = BASE_TEST_DIR / "usr/bin/app.bin";
        rbPath         = BASE_TEST_DIR / "usr/bin/rollback/app.bin";
        lastUpdateFile = BASE_TEST_DIR / "last.json";

        createDeviceConfig(CONF_FILE, 999);
        createStateMachine(lastUpdateFile, StateExecutor::FINALIZING);

        fs::create_directories(STAGING_DIR);
        stateMachine->context.busyResources.stagingDirCreated = 1;
    }
};


TEST_F(FinalizingExecutorIntegrationTest, ExecuteSuccessCommitsUpdateAndCleansUp)
{
    stateMachine->context.rollback = false;
    stateMachine->context.manifest.release.version = "2.0.0";

    fs::create_directories(rbPath.parent_path());
    writeFile(rbPath, "OLD_STABLE_VERSION");
    stateMachine->context.pathToRollbackPathMap[appPath] = rbPath;
    stateMachine->context.busyResources.rollbacks = 1;

    bool reportReceived = false;
    server->Post("/report", [&](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        EXPECT_EQ(j["status"], "SUCCESS");
        EXPECT_EQ(j["current_version"], "2.0.0");
        reportReceived = true;
        res.status = httplib::StatusCode::OK_200;
    });

    startServer();
    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
    EXPECT_FALSE(fs::exists(rbPath));
    EXPECT_FALSE(fs::exists(STAGING_DIR));
    EXPECT_EQ(stateMachine->context.busyResources.rollbacks, 0);
    EXPECT_TRUE(reportReceived);
}


TEST_F(FinalizingExecutorIntegrationTest, ExecuteRollbackRestoresFilesAndReportsFault)
{
    stateMachine->context.rollback = true;
    stateMachine->context.reportMessage = {3, "Some Error"};

    writeFile(appPath, "CORRUPTED_NEW_VERSION");
    fs::create_directories(rbPath.parent_path());
    writeFile(rbPath, "ORIGINAL_VERSION");

    stateMachine->context.pathToRollbackPathMap[appPath] = rbPath;
    stateMachine->context.busyResources.rollbacks = 1;

    server->Post("/report", [&](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        EXPECT_EQ(j["status"], "FAULT");
        EXPECT_EQ(j["error"]["code"], 3);
        res.status = httplib::StatusCode::OK_200;
    });

    startServer();
    stateMachine->run();

    EXPECT_EQ(readFileBinary(appPath), "ORIGINAL_VERSION");
    EXPECT_FALSE(fs::exists(rbPath));
    EXPECT_EQ(stateMachine->context.busyResources.rollbacks, 0);
}


TEST_F(FinalizingExecutorIntegrationTest, ExecuteRollbackLaunchesScript)
{
    stateMachine->context.rollback = true;

    ArtifactManifest::File rbScript;
    rbScript.installPath = "rollback.sh";
    stateMachine->context.manifest.files.push_back(rbScript);

    fs::path scriptPath = STAGING_DIR / "rollback.sh";
    fs::path marker = BASE_TEST_DIR / "rolled_back.marker";
    writeFile(scriptPath, "#!/bin/bash\ntouch " + marker.string());

    server->Post("/report", [](const auto&, auto& res) { res.status = httplib::StatusCode::OK_200; });
    startServer();

    stateMachine->run();

    EXPECT_TRUE(fs::exists(marker));
}


TEST_F(FinalizingExecutorIntegrationTest, ExecuteReportRetriesOnServerDown)
{
    stateMachine->context.rollback = false;
    int callCount = 0;

    server->Post("/report", [&](const httplib::Request&, httplib::Response& res) {
        callCount++;
        if (callCount < 3) {
            res.status = httplib::StatusCode::InternalServerError_500;
        } else {
            res.status = httplib::StatusCode::OK_200;
        }
    });

    startServer();
    stateMachine->run();

    EXPECT_EQ(callCount, 3);
    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
}


TEST_F(FinalizingExecutorIntegrationTest, ExecuteRollbackAlreadyRecovered)
{
    stateMachine->context.rollback = true;
    stateMachine->context.recovering = true;

    writeFile(appPath, "RECOVERED_CONTENT");
    stateMachine->context.pathToRollbackPathMap[appPath] = rbPath;

    server->Post("/report", [](const auto&, auto& res) { res.status = httplib::StatusCode::OK_200; });
    startServer();

    stateMachine->run();

    EXPECT_EQ(readFileBinary(appPath), "RECOVERED_CONTENT");
    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
}


TEST_F(FinalizingExecutorIntegrationTest, ExecuteCleanupFailureForcesReboot)
{
    stateMachine->context.rollback = false;
    stateMachine->context.busyResources.stagingDirCreated = 1;

    fs::create_directories(STAGING_DIR);
    fs::permissions(STAGING_DIR, fs::perms::none);

    server->Post("/report", [](const auto&, auto& res) { res.status = httplib::StatusCode::OK_200; });
    startServer();

    EXPECT_EXIT(stateMachine->run(), ::testing::ExitedWithCode(EXIT_FAILURE), "");

    fs::permissions(STAGING_DIR, fs::perms::all);
}


TEST_F(FinalizingExecutorIntegrationTest, ExecuteEnvVarsAreUnsetAfterCleanup)
{
    stateMachine->context.rollback = false;
    setenv("PCO_NEW_ARTIFACTS_PATHS", "/tmp/test", 1);
    setenv("PCO_STAGING_ARTIFACTS_PATHS", "/tmp/staging", 1);

    server->Post("/report", [](const auto&, auto& res) { res.status = httplib::StatusCode::OK_200; });
    startServer();

    stateMachine->run();

    EXPECT_EQ(getenv("PCO_NEW_ARTIFACTS_PATHS"), nullptr);
    EXPECT_EQ(getenv("PCO_STAGING_ARTIFACTS_PATHS"), nullptr);
}


TEST_F(FinalizingExecutorIntegrationTest, ExecuteReportServerMaxRetriesReached)
{
    stateMachine->context.rollback = false;
    int callCount = 0;

    server->Post("/report", [&](const httplib::Request&, httplib::Response& res) {
        callCount++;
        res.status = httplib::StatusCode::ServiceUnavailable_503;
    });

    startServer();
    stateMachine->run();

    EXPECT_EQ(callCount, 4);
    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
}


TEST_F(FinalizingExecutorIntegrationTest, ExecuteRollbackScriptFailsInternalUpdateError)
{
    stateMachine->context.rollback = true;

    ArtifactManifest::File rbScript;
    rbScript.installPath = "rollback.sh";
    stateMachine->context.manifest.files.push_back(rbScript);

    fs::path scriptPath = STAGING_DIR / "rollback.sh";
    writeFile(scriptPath, "#!/bin/bash\nexit 1");

    server->Post("/report", [&](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        EXPECT_EQ(j["error"]["code"], INTERNAL_UPDATE_ERROR);
        res.status = httplib::StatusCode::OK_200;
    });

    startServer();
    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
}
