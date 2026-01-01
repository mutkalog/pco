#include "integrationfixturebase.h"


class DownloadingExecutorIntegrationTest : public IntegrationFixtureBase
{
protected:
    fs::path testArchive;
    fs::path dummyFile;
    std::string archiveContent;
    fs::path lastUpdateFile;

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();

        testArchive    = BASE_TEST_DIR / "test_update.tar.gz";
        dummyFile      = BASE_TEST_DIR / "payload.txt";
        lastUpdateFile = BASE_TEST_DIR / "last.json";

        writeFile(dummyFile, "INTEGRATION_TEST_PAYLOAD");

        std::string cmd = "tar -czf " + testArchive.string() + " -C " + BASE_TEST_DIR.string() + " payload.txt";
        if (system(cmd.c_str()) != 0) {
            throw std::runtime_error("Failed to create test archive");
        }

        archiveContent = readFileBinary(testArchive);

        createDeviceConfig(CONF_FILE, 777);
        createStateMachine(lastUpdateFile, StateExecutor::DOWNLOADING);
    }
};


TEST_F(DownloadingExecutorIntegrationTest, ExecuteSuccessExtractionTransitsToVerifying)
{
    server->Get("/download", [this](const httplib::Request& req, httplib::Response& res) {
        EXPECT_EQ(req.get_param_value("id"), "777");
        EXPECT_EQ(req.get_param_value("type"), "Raspberry Pi 4");

        res.set_content(archiveContent, "application/gzip");
        res.status = httplib::OK_200;
    });

    startServer();

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::VERIFYING);
    EXPECT_EQ(stateMachine->context.busyResources.stagingDirCreated, 1);

    EXPECT_TRUE(fs::exists(STAGING_DIR / "payload.txt"));
    EXPECT_EQ(readFileBinary(STAGING_DIR / "payload.txt"), "INTEGRATION_TEST_PAYLOAD");
}


TEST_F(DownloadingExecutorIntegrationTest, ExecuteInvalidArchiveTransitsToFinalizing)
{
    server->Get("/download", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content("THIS_IS_NOT_A_VALID_TAR_GZ_CONTENT", "text/plain");
        res.status = httplib::OK_200;
    });

    startServer();
    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_EQ(stateMachine->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(DownloadingExecutorIntegrationTest, ExecuteRetryLogicSuccessOnThirdAttempt)
{
    int callCount = 0;
    server->Get("/download", [this, &callCount](const httplib::Request&, httplib::Response& res) {
        if (++callCount < 3) {
            res.status = httplib::InternalServerError_500;
        } else {
            res.set_content(archiveContent, "application/octet-stream");
            res.status = httplib::OK_200;
        }
    });

    startServer();
    stateMachine->run();

    EXPECT_EQ(callCount, 3);
    EXPECT_EQ(stateMachine->state(), StateExecutor::VERIFYING);
    EXPECT_TRUE(fs::exists(STAGING_DIR / "payload.txt"));
}


TEST_F(DownloadingExecutorIntegrationTest, ExecuteServerDownTransitsToFinalizingAfterRetries)
{
    int callCount = 0;
    server->Get("/download", [&callCount](const httplib::Request&, httplib::Response& res) {
        callCount++;
        res.status = httplib::ServiceUnavailable_503;
    });

    startServer();
    stateMachine->run();

    EXPECT_EQ(callCount, 4);
    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_TRUE(stateMachine->context.rollback);
}
