#include "integrationfixturebase.h"


class CommittingExecutorIntegrationTest : public IntegrationFixtureBase
{
protected:
    fs::path commitScript;

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();
        commitScript = STAGING_DIR / "commit.sh";

        createDeviceConfig(CONF_FILE, 444);
        createStateMachine(BASE_TEST_DIR / "last.json", StateExecutor::COMMITTING);
    }
};


TEST_F(CommittingExecutorIntegrationTest, ExecuteSuccessScriptReturnsZero)
{
    fs::path markerFile = BASE_TEST_DIR / "commit_done.txt";
    std::string scriptContent = "#!/bin/bash\ntouch " + markerFile.string() + "\nexit 0";
    writeFile(commitScript, scriptContent);

    fs::permissions(commitScript, fs::perms::owner_read | fs::perms::owner_write);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_FALSE(stateMachine->context.rollback);

    EXPECT_TRUE(fs::exists(markerFile));

    auto status = fs::status(commitScript);
    EXPECT_TRUE((status.permissions() & fs::perms::owner_exec) != fs::perms::none);
}


TEST_F(CommittingExecutorIntegrationTest, ExecuteScriptFailsReturnsError)
{
    std::string scriptContent = "#!/bin/bash\nexit 1";
    writeFile(commitScript, scriptContent);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_TRUE(stateMachine->context.rollback);
    EXPECT_EQ(stateMachine->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
    EXPECT_NE(stateMachine->context.reportMessage.second.find("returned 1"), std::string::npos);
}


TEST_F(CommittingExecutorIntegrationTest, ExecuteScriptMissingTransitsToFinalizing)
{
    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_TRUE(stateMachine->context.rollback);
}


TEST_F(CommittingExecutorIntegrationTest, ExecuteScriptWithEnvVars)
{
    setenv("PCO_COMMIT_VAR", "WORLD", 1);

    fs::path resultFile = BASE_TEST_DIR / "env_commit_result.txt";
    std::string scriptContent = "#!/bin/bash\necho $PCO_COMMIT_VAR > " + resultFile.string() + "\nexit 0";
    writeFile(commitScript, scriptContent);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_EQ(readFileBinary(resultFile), "WORLD\n");
}
