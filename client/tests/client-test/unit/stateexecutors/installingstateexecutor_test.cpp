#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <fstream>

#include <mocks/cryptoutils_mock.h>
#include <tests/client-test/unit/mocks/stateexecutor_mock.h>
#include <tests/client-test/unit/mocks/statepersistence_mock.h>
#include <tests/client-test/unit/mocks/deviceinfo_mock.h>
#include <tests/client-test/unit/mocks/httpclient_mock.h>
#include <tests/client-test/unit/stateexecutors/executorsfixturebase.h>

#include "core/stateexecutors/installingstateexecutor.h"



namespace {
const fs::path testDevConfFile            = "/tmp/devconf.json";
const fs::path testLastUpdateFile         = "/tmp/last-update.json";
const fs::path testLastUpdateRecoveryFile = testLastUpdateFile.parent_path() / "rollback" / testLastUpdateFile.filename();
const fs::path badDevConfFile             = "/fakedir/devconf.json";
const fs::path badLastUpdateFile          = "/fakedir/last-update.json";

const std::string file0Path         = "/test/myapp/app1";
const std::string file0Hash         = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";
const std::string file1Path         = "/test/myapp/app2";
const std::string file1Hash         = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";
const std::string file2Path         = "prepare.sh";
const std::string file2Hash         = "b759349b9e633738f4b8047bba51a62035e363f6e16379744a7f7840bda237aa";
const bool        file2Script       = true;
const std::string file3Path         = "commit.sh";
const std::string file3Hash         = "857256cbded0a1789906bb0ac93a3ac32bd7ff90746e6964d997a9468938d5ef";
const bool        file3Script       = true;
const std::string releaseArch       = "ARMv8";
const std::string releasePlatform   = "Linux";
const std::string releaseType       = "Raspberry Pi 4";
const std::string releaseVersion    = "1.2.1";
const std::string releaseVersionNew = "1.3.0";
const std::string releaseTimestamp  = "2025-11-12T10:23:00Z";
const std::string stagingDir        = "/tmp/pco/";


enum : uint32_t {START_STATE, USUAL_STATE};

auto createManifest = [](const std::string& version) {
    ArtifactManifest m;
    ArtifactManifest::File f0; f0.isScript = false; f0.installPath = file0Path; m.files.push_back(f0);
    ArtifactManifest::File f1; f1.isScript = false; f1.installPath = file1Path; m.files.push_back(f1);
    ArtifactManifest::File f2; f2.isScript = true;  f2.installPath = file2Path; m.files.push_back(f2);
    ArtifactManifest::File f3; f3.isScript = true;  f3.installPath = file3Path; m.files.push_back(f3);
    m.release = {version, releaseType, releasePlatform, releaseArch, tm{}};
    return m;
};

const ArtifactManifest oldManifest = createManifest(releaseVersion);
const ArtifactManifest newManifest = createManifest(releaseVersionNew);

} // namespace


using namespace testing;


class InstallingStateExecutorAllPublic : public InstallingStateExecutor
{
public:
    using InstallingStateExecutor::createNewArtifatctsPathsVar;
    using InstallingStateExecutor::installAtomic;
    using InstallingStateExecutor::createRollback;

    InstallingStateExecutorAllPublic() : InstallingStateExecutor(INSTALLING) {}
};


class InstallingStateExecutorSpy : public InstallingStateExecutorAllPublic
{
public:
    MOCK_METHOD(void, createNewArtifatctsPathsVar, (const UpdateContext &ctx), (override));
    MOCK_METHOD(void, installAtomic, (const fs::path& srcStaging, const fs::path& destPath), (override));
    MOCK_METHOD((std::pair<fs::path, fs::path>), createRollback, (const fs::path& file), (override));
};


class InstallingStateExecutorTestFixture : public ExecutorsFixtureBase
{
protected:
    fs::path testRoot = fs::temp_directory_path() / "test";

    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::INSTALLING, std::make_unique<InstallingStateExecutorAllPublic>());
        idToStateMap.emplace(StateExecutor::COMMITTING, std::move(targetSe));
        idToStateMap.emplace(StateExecutor::FINALIZING, std::move(failSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), nullptr, std::move(mockCryptoUtils), nullptr, nullptr, stagingDir, ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }

    void TearDown() override
    {
        fs::remove_all(testRoot);
    }
};


TEST_F(InstallingStateExecutorTestFixture, InstallAtomicSuccess)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());

    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    fs::remove_all(testRoot);

    fs::path stagingDir = testRoot / "staging";
    fs::path destDir    = testRoot / "dest";
    fs::path srcFile    = stagingDir / "file.bin";
    fs::path destFile   = destDir / "file.bin";

    fs::create_directories(stagingDir);

    const std::string content = "hello atomic install";

    {
        std::ofstream ofs(srcFile, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << content;
        ofs.flush();
    }

    ASSERT_NO_THROW(installingExecutor->installAtomic(srcFile, destFile));

    ASSERT_TRUE(fs::exists(destFile));
    ASSERT_FALSE(fs::exists(destDir / ".file.bin.tmp"));

    std::string result;
    std::ifstream ifs(destFile, std::ios::binary);
    ASSERT_TRUE(ifs.is_open());
    result.assign(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    );

    ASSERT_EQ(result, content);
}


TEST_F(InstallingStateExecutorTestFixture, CreateRollbackSuccess)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    fs::path destDir      = testRoot / "dest";
    fs::path destFile     = destDir / "file.bin";
    fs::path rollbackFile = destFile.parent_path() / "rollback" / destFile.filename();

    fs::create_directories(destDir);

    const std::string content = "hello rollback creation";
    {
        std::ofstream ofs(destFile, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << content;
        ofs.flush();
    }

    std::pair<fs::path, fs::path> rollbackPaths;
    ASSERT_NO_THROW({
        rollbackPaths = installingExecutor->createRollback(destFile);
    });

    ASSERT_FALSE(fs::exists(destFile));
    ASSERT_TRUE(fs::exists(rollbackFile));

    std::string result;
    std::ifstream ifs(rollbackFile, std::ios::binary);
    ASSERT_TRUE(ifs.is_open());
    result.assign(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    );

    ASSERT_EQ(result, content);
    ASSERT_EQ(rollbackPaths.first, destFile);
    ASSERT_EQ(rollbackPaths.second, rollbackFile);
}


TEST_F(InstallingStateExecutorTestFixture, CreateRollbackAlreadyExists)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    fs::path destDir      = testRoot / "dest";
    fs::path destFile     = destDir / "file.bin";
    fs::path rollbackFile = destFile.parent_path() / "rollback" / destFile.filename();

    fs::create_directories(rollbackFile.parent_path());

    const std::string content = "hello rollback creation";

    {
        std::ofstream ofs(rollbackFile, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << content;
        ofs.flush();
    }

    std::pair<fs::path, fs::path> rollbackPaths;
    ASSERT_NO_THROW({
        rollbackPaths = installingExecutor->createRollback(destFile);
    });

    ASSERT_FALSE(fs::exists(destFile));
    ASSERT_TRUE(fs::exists(rollbackFile));

    std::string result;
    std::ifstream ifs(rollbackFile, std::ios::binary);
    ASSERT_TRUE(ifs.is_open());
    result.assign(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    );

    ASSERT_EQ(result, content);
    ASSERT_EQ(rollbackPaths.first, destFile);
    ASSERT_EQ(rollbackPaths.second, rollbackFile);
}


TEST_F(InstallingStateExecutorTestFixture, CreateRollbackFileNotExists)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    fs::path destDir      = testRoot / "dest";
    fs::path destFile     = destDir / "file.bin";
    fs::path rollbackFile = destFile.parent_path() / "rollback" / destFile.filename();

    fs::create_directories(destDir);

    std::pair<fs::path, fs::path> rollbackPaths;
    ASSERT_THROW(installingExecutor->createRollback(destFile), std::system_error);

    ASSERT_FALSE(fs::exists(destFile));
    ASSERT_FALSE(fs::exists(rollbackFile));
}


TEST_F(InstallingStateExecutorTestFixture, CreateNewArtifatctsPathsVarSuccsess)
{
    std::string key = "PCO_NEW_ARTIFACTS_PATHS";
    std::string value = file0Path + ":" + file1Path;

    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    sm->context.manifest = newManifest;

    std::pair<fs::path, fs::path> rollbackPaths;
    ASSERT_NO_THROW(installingExecutor->createNewArtifatctsPathsVar(sm->context));

    const char* envValue = ::getenv(key.c_str());
    ASSERT_NE(envValue, nullptr);
    ASSERT_STREQ(envValue, value.c_str());

    EXPECT_EQ(::unsetenv(key.c_str()), 0);
}


TEST_F(InstallingStateExecutorTestFixture, InstallAtomicCreatesDestinationDirectory)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    fs::remove_all(testRoot);

    fs::path stagingDir = testRoot / "staging";
    fs::path destDir    = testRoot / "nonexistent" / "deep" / "path";
    fs::path srcFile    = stagingDir / "file.bin";
    fs::path destFile   = destDir / "file.bin";

    fs::create_directories(stagingDir);

    const std::string content = "test content";
    {
        std::ofstream ofs(srcFile, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << content;
    }

    ASSERT_FALSE(fs::exists(destDir));

    ASSERT_NO_THROW(installingExecutor->installAtomic(srcFile, destFile));

    ASSERT_TRUE(fs::exists(destDir));
    ASSERT_TRUE(fs::exists(destFile));
}


TEST_F(InstallingStateExecutorTestFixture, InstallAtomicThrowsWhenSourceNotExists)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    fs::remove_all(testRoot);

    fs::path srcFile  = testRoot / "nonexistent.bin";
    fs::path destFile = testRoot / "dest" / "file.bin";

    ASSERT_FALSE(fs::exists(srcFile));

    ASSERT_THROW(installingExecutor->installAtomic(srcFile, destFile), std::system_error);
}


TEST_F(InstallingStateExecutorTestFixture, InstallAtomicOverwritesExistingFile)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    fs::remove_all(testRoot);

    fs::path stagingDir = testRoot / "staging";
    fs::path destDir    = testRoot / "dest";
    fs::path srcFile    = stagingDir / "file.bin";
    fs::path destFile   = destDir / "file.bin";

    fs::create_directories(stagingDir);
    fs::create_directories(destDir);

    const std::string oldContent = "old content";
    const std::string newContent = "new content";

    {
        std::ofstream ofs(destFile, std::ios::binary);
        ofs << oldContent;
    }

    {
        std::ofstream ofs(srcFile, std::ios::binary);
        ofs << newContent;
    }

    ASSERT_NO_THROW(installingExecutor->installAtomic(srcFile, destFile));

    std::string result;
    std::ifstream ifs(destFile, std::ios::binary);
    result.assign(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    );

    ASSERT_EQ(result, newContent);
}


TEST_F(InstallingStateExecutorTestFixture, CreateNewArtifactsPathsVarWithSingleFile)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    ArtifactManifest manifest;
    ArtifactManifest::File f;
    f.isScript = false;
    f.installPath = "/single/path/file.bin";
    manifest.files.push_back(f);

    sm->context.manifest = manifest;

    ASSERT_NO_THROW(installingExecutor->createNewArtifatctsPathsVar(sm->context));

    const char* envValue = ::getenv("PCO_NEW_ARTIFACTS_PATHS");
    ASSERT_NE(envValue, nullptr);
    ASSERT_STREQ(envValue, "/single/path/file.bin");

    EXPECT_EQ(::unsetenv("PCO_NEW_ARTIFACTS_PATHS"), 0);
}


TEST_F(InstallingStateExecutorTestFixture, CreateNewArtifactsPathsVarSkipsScripts)
{
    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    ArtifactManifest manifest;

    ArtifactManifest::File f1;
    f1.isScript = false;
    f1.installPath = "/path/binary";
    manifest.files.push_back(f1);

    ArtifactManifest::File f2;
    f2.isScript = true;
    f2.installPath = "/path/script.sh";
    manifest.files.push_back(f2);

    ArtifactManifest::File f3;
    f3.isScript = false;
    f3.installPath = "/path/another";
    manifest.files.push_back(f3);

    sm->context.manifest = manifest;

    ASSERT_NO_THROW(installingExecutor->createNewArtifatctsPathsVar(sm->context));

    const char* envValue = ::getenv("PCO_NEW_ARTIFACTS_PATHS");
    ASSERT_NE(envValue, nullptr);
    ASSERT_STREQ(envValue, "/path/binary:/path/another");

    EXPECT_EQ(::unsetenv("PCO_NEW_ARTIFACTS_PATHS"), 0);
}



class InstallingStateExecutorTestFixtureSpy : public InstallingStateExecutorTestFixture
{
protected:
    fs::path testRoot = fs::temp_directory_path() / "test";

    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<MockStateExecutor>());
        idToStateMap.emplace(StateExecutor::INSTALLING, std::make_unique<InstallingStateExecutorSpy>());
        idToStateMap.emplace(StateExecutor::COMMITTING, std::move(targetSe));
        idToStateMap.emplace(StateExecutor::FINALIZING, std::move(failSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), nullptr, std::move(mockCryptoUtils), nullptr, nullptr, stagingDir, ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }

    void TearDown() override
    {
        fs::remove_all(testRoot);
    }
};


TEST_F(InstallingStateExecutorTestFixtureSpy, ExecuteSuccessTransitionsToCommitting)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::COMMITTING));

    sm->context.manifest = newManifest;

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(Return(oldManifest));

    sm->context.prevManifestPath = testRoot / "fake-last-update-json.json";

    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    auto nonScriptsCountOld =
        std::count_if(oldManifest.files.begin(), oldManifest.files.end(),
                                         [](const auto &v)
                                            { return !v.isScript; });

    auto nonScriptsCountNew =
        std::count_if(newManifest.files.begin(), newManifest.files.end(),
                                         [](const auto &v) { return !v.isScript; });

    EXPECT_CALL(*installingExecutor, createRollback(_))
        .Times(nonScriptsCountOld + 1)
        .WillRepeatedly([](const fs::path& p) {
            fs::path rbp = p.parent_path() / "rollback" / p.filename();
            return std::make_pair(p, rbp);
        });

    EXPECT_CALL(*installingExecutor, installAtomic(_, _))
        .Times(nonScriptsCountNew);

    EXPECT_CALL(*installingExecutor, createNewArtifatctsPathsVar(testing::_))
        .Times(1);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    installingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::COMMITTING);
    ASSERT_EQ(sm->context.busyResources.rollbacks, 1);
    ASSERT_FALSE(sm->context.rollback);
}


TEST_F(InstallingStateExecutorTestFixtureSpy, ExecuteInstallAtomicThrowsTransitionsToFinalizing)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    sm->context.manifest = newManifest;

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(Return(oldManifest));

    sm->context.prevManifestPath = testRoot / "fake-last-update-json.json";

    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    auto nonScriptsCountOld =
        std::count_if(oldManifest.files.begin(), oldManifest.files.end(),
                                         [](const auto &v)
                                            { return !v.isScript; });

    EXPECT_CALL(*installingExecutor, createRollback(_))
        .Times(nonScriptsCountOld + 1)
        .WillRepeatedly([](const fs::path& p) {
            fs::path rbp = p.parent_path() / "rollback" / p.filename();
            return std::make_pair(p, rbp);
        });

    EXPECT_CALL(*installingExecutor, installAtomic(testing::_, testing::_))
        .WillOnce(testing::Throw(std::runtime_error("install failed")));

    EXPECT_CALL(*installingExecutor, createNewArtifatctsPathsVar(testing::_))
        .Times(0);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    installingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_EQ(sm->context.busyResources.rollbacks, 1);
    ASSERT_TRUE(sm->context.rollback);
}


TEST_F(InstallingStateExecutorTestFixtureSpy, ExecuteSuccessFirstInstall)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::COMMITTING));

    sm->context.manifest = newManifest;

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(Return(ArtifactManifest{}));

    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    auto nonScriptsCountNew =
        std::count_if(newManifest.files.begin(), newManifest.files.end(),
                                         [](const auto &v) { return !v.isScript; });

    EXPECT_CALL(*installingExecutor, createRollback(_))
        .Times(0);

    EXPECT_CALL(*installingExecutor, installAtomic(_, _))
        .Times(nonScriptsCountNew);

    EXPECT_CALL(*installingExecutor, createNewArtifatctsPathsVar(testing::_))
        .Times(1);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    installingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::COMMITTING);
    ASSERT_EQ(sm->context.busyResources.rollbacks, 0);
    ASSERT_FALSE(sm->context.rollback);
}



TEST_F(InstallingStateExecutorTestFixtureSpy, ExecuteCreateRollbackThrowsTransitionsToFinalizing)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(Return(StateExecutor::FINALIZING));

    sm->context.manifest = newManifest;

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(Return(oldManifest));

    sm->context.prevManifestPath = testRoot / "fake-last-update.json";

    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    EXPECT_CALL(*installingExecutor, createRollback(_))
        .WillOnce(Throw(std::system_error(std::make_error_code(std::errc::no_such_file_or_directory))));

    EXPECT_CALL(*installingExecutor, installAtomic(_, _))
        .Times(0);

    EXPECT_CALL(*installingExecutor, createNewArtifatctsPathsVar(_))
        .Times(0);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    installingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
}


TEST_F(InstallingStateExecutorTestFixtureSpy, ExecuteWithOnlyScriptsInManifest)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(Return(StateExecutor::COMMITTING));

    ArtifactManifest scriptsOnlyManifest;
    ArtifactManifest::File script1;
    script1.isScript = true;
    script1.installPath = "prepare.sh";
    scriptsOnlyManifest.files.push_back(script1);

    ArtifactManifest::File script2;
    script2.isScript = true;
    script2.installPath = "commit.sh";
    scriptsOnlyManifest.files.push_back(script2);

    sm->context.manifest = scriptsOnlyManifest;

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(Return(ArtifactManifest{}));

    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    EXPECT_CALL(*installingExecutor, createRollback(_))
        .Times(0);

    EXPECT_CALL(*installingExecutor, installAtomic(_, _))
        .Times(0);

    EXPECT_CALL(*installingExecutor, createNewArtifatctsPathsVar(_))
        .Times(1);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    installingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::COMMITTING);
    ASSERT_FALSE(sm->context.rollback);
}


TEST_F(InstallingStateExecutorTestFixtureSpy, ExecutePartialInstallFailureCreatesRollbacksBeforeFailure)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(Return(StateExecutor::FINALIZING));

    sm->context.manifest = newManifest;

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(Return(oldManifest));

    sm->context.prevManifestPath = testRoot / "fake-last-update.json";

    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    auto nonScriptsCountOld =
        std::count_if(oldManifest.files.begin(), oldManifest.files.end(),
                      [](const auto& v) { return !v.isScript; });

    EXPECT_CALL(*installingExecutor, createRollback(_))
        .Times(nonScriptsCountOld + 1)
        .WillRepeatedly([](const fs::path& p) {
            fs::path rbp = p.parent_path() / "rollback" / p.filename();
            return std::make_pair(p, rbp);
        });

    EXPECT_CALL(*installingExecutor, installAtomic(_, _))
        .WillOnce(Return())
        .WillOnce(Throw(std::runtime_error("disk full")));

    EXPECT_CALL(*installingExecutor, createNewArtifatctsPathsVar(_))
        .Times(0);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    installingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.busyResources.rollbacks, 1);
}


TEST_F(InstallingStateExecutorTestFixtureSpy, ExecuteEmptyManifestTransitionsToCommitting)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(Return(StateExecutor::COMMITTING));

    ArtifactManifest emptyManifest;
    sm->context.manifest = emptyManifest;

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(Return(ArtifactManifest{}));

    auto it = sm->idToStateMap_.find(StateExecutor::INSTALLING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto installingExecutor = dynamic_cast<InstallingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(installingExecutor, nullptr);

    EXPECT_CALL(*installingExecutor, createRollback(_))
        .Times(0);

    EXPECT_CALL(*installingExecutor, installAtomic(_, _))
        .Times(0);

    EXPECT_CALL(*installingExecutor, createNewArtifatctsPathsVar(_))
        .Times(1);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    installingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::COMMITTING);
    ASSERT_FALSE(sm->context.rollback);
    ASSERT_EQ(sm->context.busyResources.rollbacks, 0);
}
