#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>

#include "core/service/uploadservice.h"
#include "mocks/archivetools_mock.h"
#include "mocks/cryptoutils_mock.h"

#include "servicefixturebase.h"


namespace fs = std::filesystem;
using json = nlohmann::ordered_json;
using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;


class UploadServiceTest : public ServiceFixtureBase
{
protected:
    std::string testId;
    fs::path bufferDir;
    fs::path storageDir;
    MockArchiveTools* mockArchivePtr;
    MockCryptoUtils* mockCryptoPtr;
    std::unique_ptr<UploadService> service;

    void callParseManifest(const std::string& raw,
                           ReleasesTableEntry& entry)
    {
        service->parseManifest(raw, entry);
    }

    void callParseFiles(std::shared_ptr<ServerContext>& sc,
                        const std::string& raw,
                        ReleasesTableEntry& entry,
                        const fs::path& bufDir,
                        fs::path& storagePath)
    {
        service->parseFiles(sc, raw, entry, bufDir, storagePath);
    }

    void callCommit(std::shared_ptr<ServerContext>& sc,
                    std::optional<int> canaryPercentage,
                    int requiredTimeMinutes,
                    ReleasesTableEntry& entry,
                    const fs::path& bufDir)
    {
        service->commit(sc, canaryPercentage, requiredTimeMinutes, entry, bufDir);
    }


    void SetUp() override
    {
        Database::instance(testDbname, "postgres", "127.0.0.1", "5433");

        testId     = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        bufferDir  = fs::temp_directory_path() / ("test_buffer_" + testId);
        storageDir = fs::temp_directory_path() / ("test_storage_" + testId);

        fs::remove_all(bufferDir);
        fs::remove_all(storageDir);
        fs::create_directories(bufferDir);
        fs::create_directories(storageDir);

        auto mockArchive = std::make_unique<NiceMock<MockArchiveTools>>();
        auto mockCrypto  = std::make_unique<NiceMock<MockCryptoUtils>>();

        mockArchivePtr = mockArchive.get();
        mockCryptoPtr  = mockCrypto.get();

        sc = std::make_shared<ServerContext>(
            std::move(mockCrypto),
            std::move(mockArchive)
        );

        service = std::make_unique<UploadService>(bufferDir, storageDir);
    }

    void TearDown() override
    {
        fs::remove_all(bufferDir);
        fs::remove_all(storageDir);

        clearTables();
    }

    std::string createValidManifestJson(
        const std::string& version = "1.0.0",
        const std::string& type = "app",
        const std::string& platform = "linux",
        const std::string& arch = "x64")
    {
        json manifest;
        manifest["release"]["version"] = version;
        manifest["release"]["type"] = type;
        manifest["release"]["platform"] = platform;
        manifest["release"]["arch"] = arch;

        json data;
        data["manifest"] = manifest.dump();
        data["signature"] = "base64_signature_here";

        return data.dump();
    }

    void createTestFilesInDir(const fs::path& dir, const std::vector<std::string>& filenames)
    {
        fs::create_directories(dir);
        for (const auto& name : filenames)
        {
            std::ofstream(dir / name) << "test content for " << name;
        }
    }
};


TEST_F(UploadServiceTest, ParseManifestEmptyStringThrows)
{
    ReleasesTableEntry entry;
    ASSERT_THROW(callParseManifest("", entry), std::runtime_error);
}


TEST_F(UploadServiceTest, ParseManifestInvalidJsonThrows)
{
    ReleasesTableEntry entry;
    ASSERT_THROW(callParseManifest("{invalid json", entry), json::parse_error);
}


TEST_F(UploadServiceTest, ParseManifestMissingManifestThrows)
{
    ReleasesTableEntry entry;
    json data;
    data["signature"] = "sig";
    ASSERT_THROW(callParseManifest(data.dump(), entry), std::runtime_error);
}


TEST_F(UploadServiceTest, ParseManifestManifestNotStringThrows)
{
    ReleasesTableEntry entry;
    json data;
    data["manifest"] = 123;
    data["signature"] = "sig";
    ASSERT_THROW(callParseManifest(data.dump(), entry), std::runtime_error);
}


TEST_F(UploadServiceTest, ParseManifestValidParsesFields)
{
    ReleasesTableEntry entry;
    auto raw = createValidManifestJson("2.5.3", "firmware", "windows", "arm64");
    ASSERT_NO_THROW(callParseManifest(raw, entry));
    ASSERT_EQ(entry.version, "2.5.3");
    ASSERT_EQ(entry.type, "firmware");
    ASSERT_EQ(entry.platform, "windows");
    ASSERT_EQ(entry.arch, "arm64");
    ASSERT_EQ(entry.signature, "base64_signature_here");
    ASSERT_FALSE(entry.manifest.empty());
}


TEST_F(UploadServiceTest, ParseFilesExtractionFailureThrows)
{
    ReleasesTableEntry entry;
    entry.type = "app";
    entry.platform = "linux";
    entry.arch = "x64";
    entry.version = "1.0.0";

    fs::path bufDir = bufferDir / "fail";
    fs::path storagePath;

    EXPECT_CALL(*mockArchivePtr, extract(_, _, _))
        .WillOnce(Return(-1));

    ASSERT_THROW(callParseFiles(sc, "data", entry, bufDir, storagePath), std::runtime_error);
}


TEST_F(UploadServiceTest, ParseFilesSuccessfulPopulatesPaths)
{
    ReleasesTableEntry entry;
    entry.type = "app";
    entry.platform = "linux";
    entry.arch = "x64";
    entry.version = "1.0.0";

    fs::path bufDir = bufferDir / "ok";
    fs::path storagePath;

    EXPECT_CALL(*mockArchivePtr, extract(_, _, _))
        .WillOnce([](const uint8_t*, size_t, const char* outdir) {
            fs::create_directories(outdir);
            std::ofstream(fs::path(outdir) / "a.bin");
            std::ofstream(fs::path(outdir) / "b.bin");
            return 0;
        });

    ASSERT_NO_THROW(callParseFiles(sc, "data", entry, bufDir, storagePath));
    ASSERT_EQ(entry.storagePaths.size(), 2);
    ASSERT_EQ(entry.bufferPathToStoragePath.size(), 2);
    ASSERT_FALSE(storagePath.empty());
}


TEST_F(UploadServiceTest, ParseFilesNoRegularFilesLeavesEmptyPaths)
{
    ReleasesTableEntry entry;
    entry.type = "app";
    entry.platform = "linux";
    entry.arch = "x64";
    entry.version = "1.0.0";

    fs::path bufDir = bufferDir / "empty";
    fs::path storagePath;

    EXPECT_CALL(*mockArchivePtr, extract(_, _, _))
        .WillOnce([](const uint8_t*, size_t, const char* outdir) {
            fs::create_directories(fs::path(outdir) / "dir");
            return 0;
        });

    ASSERT_NO_THROW(callParseFiles(sc, "data", entry, bufDir, storagePath));
    ASSERT_TRUE(entry.storagePaths.empty());
}


TEST_F(UploadServiceTest, ParseFilesBuildsCorrectStoragePath)
{
    ReleasesTableEntry entry;
    entry.type = "firmware";
    entry.platform = "macos";
    entry.arch = "arm64";
    entry.version = "3.2.1";

    fs::path bufDir = bufferDir / "paths";
    fs::path storagePath;

    EXPECT_CALL(*mockArchivePtr, extract(_, _, _))
        .WillOnce([](const uint8_t*, size_t, const char* outdir) {
            fs::create_directories(outdir);
            std::ofstream(fs::path(outdir) / "update.pkg");
            return 0;
        });

    callParseFiles(sc, "data", entry, bufDir, storagePath);

    fs::path expected = storageDir / "firmware" / "arm64" / "macos" / "3.2.1";
    ASSERT_EQ(storagePath, expected);
    ASSERT_EQ(entry.storagePaths[0], expected / "update.pkg");
}


TEST_F(UploadServiceTest, ParseFilesCleansExistingBufferDir)
{
    ReleasesTableEntry entry;
    entry.type = "app";
    entry.platform = "linux";
    entry.arch = "x64";
    entry.version = "1.0.0";

    fs::path bufDir = bufferDir / "cleanup";
    fs::path storagePath;

    fs::create_directories(bufDir);
    std::ofstream(bufDir / "old");

    ASSERT_TRUE(fs::exists(bufDir / "old"));

    EXPECT_CALL(*mockArchivePtr, extract(_, _, _))
        .WillOnce([](const uint8_t*, size_t, const char* outdir) {
            std::ofstream(fs::path(outdir) / "new");
            return 0;
        });

    callParseFiles(sc, "data", entry, bufDir, storagePath);

    ASSERT_FALSE(fs::exists(bufDir / "old"));
    ASSERT_TRUE(fs::exists(bufDir / "new"));
}


TEST_F(UploadServiceTest, UploadEmptyManifestThrows)
{
    ASSERT_THROW(service->upload(sc, std::nullopt, 30, "", "data"), std::runtime_error);
}


TEST_F(UploadServiceTest, UploadInvalidManifestThrows)
{
    ASSERT_THROW(service->upload(sc, std::nullopt, 30, "not json", "data"), json::parse_error);
}


TEST_F(UploadServiceTest, commitEmptyStoragePathsThrows)
{
    ReleasesTableEntry entry;
    fs::path bufDir = bufferDir / "commit";
    ASSERT_THROW(callCommit(sc, std::nullopt, 30, entry, bufDir), std::runtime_error);
}


TEST_F(UploadServiceTest, CommitWithCanaryCreatesCanaryRollout)
{
    ReleasesTableEntry entry;
    entry.manifest = R"({"release":{"version":"c1"}})";
    entry.signature = "sig";
    entry.version = "c1";
    entry.type = "app";
    entry.platform = "linux";
    entry.arch = "x64";

    fs::path bufDir = bufferDir / "canary";
    fs::create_directories(bufDir);
    fs::path storage = storageDir / "app" / "x64" / "linux" / "c1";
    fs::create_directories(storage);

    std::ofstream(bufDir / "f.bin");
    entry.bufferPathToStoragePath.push_back({bufDir / "f.bin", storage});
    entry.storagePaths.push_back(storage / "f.bin");

    ASSERT_NO_THROW(callCommit(sc, 25, 60, entry, bufDir));

    auto& rollouts = sc->staging.rollouts;
    std::lock_guard<std::mutex> lg(rollouts.mtx);

    bool found = false;
    for (auto& [_, info] : rollouts.releaseToInfoMap)
    {
        if (info.type == "app")
        {
            found = true;
            ASSERT_TRUE(info.isCanary);
            ASSERT_EQ(info.inRolloutPercentage, 25);
            break;
        }
    }
    ASSERT_TRUE(found);
}


TEST_F(UploadServiceTest, CommitWithoutCanaryCreatesNonCanaryRollout)
{
    ReleasesTableEntry entry;
    entry.manifest = R"({"release":{"version":"n1"}})";
    entry.signature = "sig";
    entry.version = "n1";
    entry.type = "fw";
    entry.platform = "windows";
    entry.arch = "arm64";

    fs::path bufDir = bufferDir / "nocanary";
    fs::create_directories(bufDir);
    fs::path storage = storageDir / "fw" / "arm64" / "windows" / "n1";
    fs::create_directories(storage);

    std::ofstream(bufDir / "f.bin");
    entry.bufferPathToStoragePath.push_back({bufDir / "f.bin", storage});
    entry.storagePaths.push_back(storage / "f.bin");

    ASSERT_NO_THROW(callCommit(sc, std::nullopt, 30, entry, bufDir));

    auto& rollouts = sc->staging.rollouts;
    std::lock_guard<std::mutex> lg(rollouts.mtx);

    bool found = false;
    for (auto& [_, info] : rollouts.releaseToInfoMap)
    {
        if (info.type == "fw")
        {
            found = true;
            ASSERT_FALSE(info.isCanary);
            ASSERT_EQ(info.inRolloutPercentage, 0);
            break;
        }
    }

    ASSERT_TRUE(found);
}
