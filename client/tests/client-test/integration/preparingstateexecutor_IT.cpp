#include "integrationfixturebase.h"


class PreparingExecutorIntegrationTest : public IntegrationFixtureBase
{
protected:
    fs::path prepareScript;

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();
        prepareScript = STAGING_DIR / "prepare.sh";

        createDeviceConfig(CONF_FILE, 333);
        createStateMachine(BASE_TEST_DIR / "last.json", StateExecutor::PREPARING);
    }
};


TEST_F(PreparingExecutorIntegrationTest, ExecuteSuccessScriptReturnsZero)
{
    fs::path markerFile = BASE_TEST_DIR / "prepare_done.txt";
    std::string scriptContent = "#!/bin/bash\ntouch " + markerFile.string() + "\nexit 0";
    writeFile(prepareScript, scriptContent);

    fs::permissions(prepareScript, fs::perms::owner_read | fs::perms::owner_write);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::INSTALLING);

    EXPECT_TRUE(fs::exists(markerFile));

    auto status = fs::status(prepareScript);
    EXPECT_TRUE((status.permissions() & fs::perms::owner_exec) != fs::perms::none);
}


TEST_F(PreparingExecutorIntegrationTest, ExecuteScriptFailsReturnsError)
{
    std::string scriptContent = "#!/bin/bash\nexit 1";
    writeFile(prepareScript, scriptContent);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_TRUE(stateMachine->context.rollback);
    EXPECT_EQ(stateMachine->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
    EXPECT_NE(stateMachine->context.reportMessage.second.find("returned 1"), std::string::npos);
}


TEST_F(PreparingExecutorIntegrationTest, ExecuteScriptMissingTransitsToFinalizing)
{
    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_TRUE(stateMachine->context.rollback);
}


TEST_F(PreparingExecutorIntegrationTest, ExecuteScriptWithEnvVars)
{
    setenv("PCO_TEST_VAR", "HELLO", 1);

    fs::path resultFile = BASE_TEST_DIR / "env_result.txt";
    std::string scriptContent = "#!/bin/bash\necho $PCO_TEST_VAR > " + resultFile.string() + "\nexit 0";
    writeFile(prepareScript, scriptContent);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::INSTALLING);
    EXPECT_EQ(readFileBinary(resultFile), "HELLO\n");
}
