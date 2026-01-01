#include "integrationfixturebase.h"

class InstallingExecutorIntegrationTest : public IntegrationFixtureBase
{
protected:
    fs::path installDir;
    fs::path rollbackDir;
    fs::path fileInSystem;
    fs::path fileInStaging;
    fs::path lastUpdateFile;

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();

        installDir     = BASE_TEST_DIR / "usr/bin";
        rollbackDir    = installDir / "rollback";
        fileInSystem   = installDir / "app.bin";
        fileInStaging  = STAGING_DIR / "app.bin";
        lastUpdateFile = BASE_TEST_DIR / "last.json";

        createDeviceConfig(CONF_FILE, 555);
    }

    void prepareContext(const std::string& currentContent, const std::string& newContent)
    {
        writeFile(fileInSystem, currentContent);
        writeFile(fileInStaging, newContent);

        json prevManifestJson = {
            {"release", {
                {"version", "1.0.0"},
                {"type", "Raspberry Pi 4"},
                {"platform", "Linux"},
                {"arch", "ARMv8"},
                {"timestamp", "2025-01-01T12:00:00Z"}
            }},
            {"files", {{
                {"script", false},
                {"path", fileInSystem.string()},
                {"hash", {
                    {"algo", "sha256"},
                    {"value", "6f74686572"}
                }}
            }}}
        };

        writeFile(lastUpdateFile, prevManifestJson.dump());

        createStateMachine(lastUpdateFile, StateExecutor::INSTALLING);

        ArtifactManifest::File newFile;
        newFile.installPath                  = fileInSystem;
        newFile.isScript                     = false;
        stateMachine->context.manifest.files = {newFile};
    }
};


TEST_F(InstallingExecutorIntegrationTest, ExecuteSuccessAtomicInstallWithRollback)
{
    prepareContext("OLD_VERSION", "NEW_VERSION");

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::COMMITTING);

    EXPECT_TRUE(fs::exists(rollbackDir / "app.bin"));
    EXPECT_EQ(readFileBinary(rollbackDir / "app.bin"), "OLD_VERSION");

    EXPECT_TRUE(fs::exists(fileInSystem));
    EXPECT_EQ(readFileBinary(fileInSystem), "NEW_VERSION");

    const char* env = std::getenv("PCO_NEW_ARTIFACTS_PATHS");
    ASSERT_NE(env, nullptr);
    EXPECT_STREQ(env, fileInSystem.c_str());
}


TEST_F(InstallingExecutorIntegrationTest, ExecuteMissingStagingFileTransitsToFinalizing)
{
    prepareContext("OLD_VERSION", "NEW_VERSION");
    fs::remove(fileInStaging);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_TRUE(stateMachine->context.rollback);
}


TEST_F(InstallingExecutorIntegrationTest, ExecuteNoTempFilesLeftBehind)
{
    prepareContext("V1", "V2");

    stateMachine->run();

    for (const auto& entry : fs::directory_iterator(installDir))
    {
        std::string name = entry.path().filename().string();
        EXPECT_FALSE(name.find(".tmp") != std::string::npos);
    }
}


TEST_F(InstallingExecutorIntegrationTest, ExecuteMultipleFilesSuccess)
{
    fs::path file2System = installDir / "service.conf";
    fs::path file2Staging = STAGING_DIR / "service.conf";
    writeFile(file2System, "OLD_2");
    writeFile(file2Staging, "NEW_2");

    json prevManifestJson = {
        {"release", {
            {"version", "1.0.0"},
            {"type", "Raspberry Pi 4"},
            {"platform", "Linux"},
            {"arch", "ARMv8"},
            {"timestamp", "2025-01-01T12:00:00Z"}
        }},
        {"files", {
            {{"script", false}, {"path", fileInSystem.string()}, {"hash", {{"algo", "sha256"}, {"value", "aabbb12312"}}}},
            {{"script", false}, {"path", file2System.string()}, {"hash", {{"algo", "sha256"}, {"value", "dddeeefff888"}}}}
        }}
    };

    writeFile(fileInSystem, "OLD_1");
    writeFile(fileInStaging, "NEW_1");
    writeFile(lastUpdateFile, prevManifestJson.dump());

    createStateMachine(lastUpdateFile, StateExecutor::INSTALLING);

    ArtifactManifest::File f1;
    f1.installPath = fileInSystem;
    f1.isScript = false;

    ArtifactManifest::File f2;
    f2.installPath = file2System;
    f2.isScript = false;

    stateMachine->context.manifest.files = {f1, f2};

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::COMMITTING);

    EXPECT_EQ(readFileBinary(fileInSystem), "NEW_1");
    EXPECT_EQ(readFileBinary(file2System), "NEW_2");

    EXPECT_TRUE(fs::exists(rollbackDir / "app.bin"));
    EXPECT_TRUE(fs::exists(rollbackDir / "service.conf"));

    const char* env = std::getenv("PCO_NEW_ARTIFACTS_PATHS");
    ASSERT_NE(env, nullptr);
    std::string envStr(env);
    EXPECT_TRUE(envStr.find(fileInSystem.string()) != std::string::npos);
    EXPECT_TRUE(envStr.find(file2System.string()) != std::string::npos);
    EXPECT_TRUE(envStr.find(":") != std::string::npos);
}


TEST_F(InstallingExecutorIntegrationTest, ExecuteRollbackAlreadyExists)
{
    prepareContext("OLD_VERSION", "NEW_VERSION");

    fs::create_directories(rollbackDir);
    fs::path rbFile = rollbackDir / "app.bin";
    writeFile(rbFile, "EXISTING_ROLLBACK_CONTENT");

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::COMMITTING);

    EXPECT_EQ(readFileBinary(rbFile), "EXISTING_ROLLBACK_CONTENT");
    EXPECT_EQ(readFileBinary(fileInSystem), "NEW_VERSION");
}


TEST_F(InstallingExecutorIntegrationTest, ExecuteIgnoreScripts)
{
    prepareContext("OLD_APP", "NEW_APP");

    ArtifactManifest::File scriptFile;
    scriptFile.installPath = installDir / "setup.sh";
    scriptFile.isScript = true;
    stateMachine->context.manifest.files.push_back(scriptFile);

    writeFile(STAGING_DIR / "setup.sh", "#!/bin/bash");

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::COMMITTING);

    EXPECT_FALSE(fs::exists(installDir / "setup.sh"));
}


TEST_F(InstallingExecutorIntegrationTest, ExecuteRollbackSourceMissingTransitsToFinalizing)
{
    prepareContext("OLD", "NEW");

    fs::remove(fileInSystem);

    stateMachine->run();

    EXPECT_EQ(stateMachine->state(), StateExecutor::FINALIZING);
    EXPECT_TRUE(stateMachine->context.rollback);
}
