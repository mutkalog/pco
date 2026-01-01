#include "integrationfixturebase.h"

class VerifyingExecutorIntegrationTest : public IntegrationFixtureBase
{
protected:
    fs::path file1;
    fs::path file2;
    std::vector<uint8_t> hash1;
    std::vector<uint8_t> hash2;

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();

        file1 = STAGING_DIR / "app.bin";
        file2 = STAGING_DIR / "config.yaml";

        writeFile(file1, "REAL_BINARY_DATA_123");
        writeFile(file2, "SOME_CONFIG_DATA_ABC");

        SSLUtilsAdapter crypto;
        hash1 = crypto.sha256FromFile(file1);
        hash2 = crypto.sha256FromFile(file2);

        createDeviceConfig(CONF_FILE, 111);

        createStateMachine(BASE_TEST_DIR / "last.json", StateExecutor::VERIFYING);
    }

    void TearDown() override
    {
        IntegrationFixtureBase::TearDown();
        unsetenv("PCO_STAGING_ARTIFACTS_PATHS");
    }

    void fillManifest(const std::vector<std::pair<fs::path, std::vector<uint8_t>>>& files)
    {
        auto& manifest = stateMachine->context.manifest;
        manifest.files.clear();
        for (const auto& f : files)
        {
            ArtifactManifest::File item;
            item.installPath = f.first.string();
            item.hash.value = f.second;
            item.isScript = false;
            manifest.files.push_back(item);
        }
    }
};


TEST_F(VerifyingExecutorIntegrationTest, ExecuteSuccessAllHashesMatch)
{
    fillManifest({
        {fs::path("/usr/bin/app.bin"), hash1},
        {fs::path("/etc/config.yaml"), hash2}
    });

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::PREPARING);

    const char* env = std::getenv("PCO_STAGING_ARTIFACTS_PATHS");
    ASSERT_NE(env, nullptr);
    std::string envVal(env);

    EXPECT_NE(envVal.find("app.bin"), std::string::npos);
    EXPECT_NE(envVal.find("config.yaml"), std::string::npos);
}


TEST_F(VerifyingExecutorIntegrationTest, ExecuteHashMismatchTransitsToFinalizing)
{
    std::vector<uint8_t> fakeHash = {0, 1, 2, 3};
    fillManifest({
        {fs::path("/usr/bin/app.bin"), hash1},
        {fs::path("/etc/config.yaml"), fakeHash}
    });

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_TRUE(stateMachine->context.rollback);
    EXPECT_EQ(stateMachine->context.reportMessage.first, ARTIFACT_INTEGRITY_ERROR);
}


TEST_F(VerifyingExecutorIntegrationTest, ExecuteMissingFileTransitsToFinalizing)
{
    fillManifest({
        {fs::path("/usr/bin/app.bin"), hash1},
        {fs::path("/non/existent/file"), hash2}
    });

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
}


TEST_F(VerifyingExecutorIntegrationTest, ExecuteCorrectEnvVarFormatting)
{
    fillManifest({
        {fs::path("/usr/bin/app.bin"), hash1},
        {fs::path("/etc/config.yaml"), hash2}
    });

    stateMachine->run();

    std::string expected = (STAGING_DIR / "app.bin").string() + ":" + (STAGING_DIR / "config.yaml").string();
    const char* env = std::getenv("PCO_STAGING_ARTIFACTS_PATHS");
    EXPECT_STREQ(env, expected.c_str());
}
