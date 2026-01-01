#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <fstream>

#include "core/deviceconfig.h"

namespace {
const fs::path testDevConfFile            = "/tmp/devconf.json";
const fs::path testLastUpdateFile         = "/tmp/last-update.json";
const fs::path testLastUpdateRecoveryFile = testLastUpdateFile.parent_path() / "rollback" / testLastUpdateFile.filename();
const fs::path badDevConfFile             = "/fakedir/devconf.json";
const fs::path badLastUpdateFile          = "/fakedir/last-update.json";
}


class DeviceConfigTest : public ::testing::Test
{
protected:
    fs::path devConfigFile;
    fs::path lastUpdateFile;
    std::unique_ptr<DeviceConfig> devconf;

    void init(const fs::path& devConf, const fs::path& lastUpdate)
    {
        devconf = std::make_unique<DeviceConfig>(devConf, lastUpdate);
        devConfigFile  = devConf;
        lastUpdateFile = lastUpdate;
    }

    void TearDown() override
    {
        fs::remove(devConfigFile);
        fs::remove(lastUpdateFile);
        fs::remove_all(testLastUpdateRecoveryFile.parent_path());
    }

    void writeToFile(const fs::path& file, const std::string& content)
    {
        std::ofstream out(file);
        ASSERT_TRUE(out.is_open());
        out << content;
        out.flush();
    }
};

TEST_F(DeviceConfigTest, LoadValidConfigSuccess)
{
    init(testDevConfFile, "");

    std::string type                   = "Rock Pi";
    std::string platform               = "QNX";
    std::string arch                   = "ARMv9";
    int         pollingIntervalMinutes = 15;
    std::string serverUrl              = "https://example.com";
    int         serverPort             = 8080;
    fs::path    certPath               = "/tmp/cert.pem";
    fs::path    keyPath                = "/tmp/key.pem";
    fs::path    caCertPath             = "/tmp/ca.pem";
    fs::path    publicKeyPath          = "/tmp/pub.pem";
    uint32_t    id                      = 42;

    json content = {
        {"type", type},
        {"platform", platform},
        {"arch", arch},
        {"updatePollingIntervalMinutes", pollingIntervalMinutes},
        {"serverURL", serverUrl},
        {"serverPort", serverPort},
        {"certPath", certPath.string()},
        {"keyPath", keyPath.string()},
        {"caCertPath", caCertPath.string()},
        {"publicKeyPath", publicKeyPath.string()},
        {"id", id}
    };

    writeToFile(devConfigFile, content.dump());

    devconf->loadConfig();

    ASSERT_EQ(devconf->type(), type);
    ASSERT_EQ(devconf->platform(), platform);
    ASSERT_EQ(devconf->arch(), arch);
    ASSERT_EQ(devconf->pollingIntervalMinutes(), pollingIntervalMinutes);
    ASSERT_EQ(devconf->serverUrl(), serverUrl);
    ASSERT_EQ(devconf->serverPort(), serverPort);
    ASSERT_EQ(devconf->certPath(), certPath);
    ASSERT_EQ(devconf->keyPath(), keyPath);
    ASSERT_EQ(devconf->caCertPath(), caCertPath);
    ASSERT_EQ(devconf->publicKeyPath(), publicKeyPath);
    ASSERT_EQ(devconf->id(), id);
}


TEST_F(DeviceConfigTest, LoadNonexistentFileThrows)
{
    init(badDevConfFile, "");

    EXPECT_THROW(devconf->loadConfig(), std::system_error);
}


TEST_F(DeviceConfigTest, LoadInvalidJsonThrows)
{
    init(testDevConfFile, "");

    writeToFile(devConfigFile, "invalid json");

    EXPECT_THROW(devconf->loadConfig(), json::parse_error);
}

TEST_F(DeviceConfigTest, LoadLastUpdateNormalWaySuccess)
{
    init("", testLastUpdateFile);

    std::string file1Path   = "/opt/myapp/app2";
    std::string file1Hash   = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";

    std::string file2Path   = "prepare.sh";
    std::string file2Hash   = "b759349b9e633738f4b8047bba51a62035e363f6e16379744a7f7840bda237aa";
    bool        file2Script = true;

    std::string file3Path   = "commit.sh";
    std::string file3Hash   = "857256cbded0a1789906bb0ac93a3ac32bd7ff90746e6964d997a9468938d5ef";
    bool        file3Script = true;

    std::string releaseArch      = "ARMv8";
    std::string releasePlatform  = "Linux";
    std::string releaseType      = "Raspberry Pi 4";
    std::string releaseVersion   = "1.2.1";
    std::string releaseTimestamp = "2025-11-12T10:23:00Z";

    json manifest = {
        {"files", {
            {
                {"path", file1Path},
                {"hash", { {"algo", "sha256"}, {"value", file1Hash} }},
            },
            {
                {"path", file2Path},
                {"hash", { {"algo", "sha256"}, {"value", file2Hash} }},
                {"script", file2Script}
            },
            {
                {"path", file3Path},
                {"hash", { {"algo", "sha256"}, {"value", file3Hash} }},
                {"script", file3Script}
            }
        }},
        {"release", {
            {"arch", releaseArch},
            {"platform", releasePlatform},
            {"type", releaseType},
            {"version", releaseVersion},
            {"timestamp", releaseTimestamp}
        }}
    };

    writeToFile(lastUpdateFile, manifest.dump());

    devconf->loadPrevManifest();

    const auto& files = devconf->prevManifest().files;
    ASSERT_EQ(files.size(), 3u);

    ASSERT_EQ(files[0].installPath, fs::path(file1Path));
    ASSERT_EQ(files[0].hash.algo, "sha256");
    ASSERT_EQ(files[0].isScript, false);

    ASSERT_EQ(files[1].installPath, fs::path(file2Path));
    ASSERT_EQ(files[1].hash.algo, "sha256");
    ASSERT_EQ(files[1].isScript, true);

    ASSERT_EQ(files[2].installPath, fs::path(file3Path));
    ASSERT_EQ(files[2].hash.algo, "sha256");
    ASSERT_EQ(files[2].isScript, true);

    const auto& release = devconf->prevManifest().release;
    ASSERT_EQ(release.arch, releaseArch);
    ASSERT_EQ(release.platform, releasePlatform);
    ASSERT_EQ(release.type, releaseType);
    ASSERT_EQ(release.version, releaseVersion);
}


TEST_F(DeviceConfigTest, LoadLastUpdateRecoveryWaySuccess)
{
    init("", testLastUpdateFile);

    std::string file1Path   = "/opt/myapp/app2";
    std::string file1Hash   = "c93c5405894b0fa7021fc39e32e201111111111111111d2a024adf6a5bf28459";

    std::string file2Path   = "prepare.sh";
    std::string file2Hash   = "b759349b9e633738f4b8047bba51a62035e363f6e16379744a7f7840bda237aa";
    bool        file2Script = true;

    std::string file3Path   = "commit.sh";
    std::string file3Hash   = "857256cbded0a1789906bb0ac93a3ac32bd7ff90746e6964d997a9468938d5ef";
    bool        file3Script = true;

    std::string releaseArch      = "ARMv8";
    std::string releasePlatform  = "Linux";
    std::string releaseType      = "Raspberry Pi 4";
    std::string releaseVersion   = "1.2.0";
    std::string releaseTimestamp = "2025-11-12T10:23:00Z";

    json manifest = {
        {"files", {
            {
                {"path", file1Path},
                {"hash", { {"algo", "sha256"}, {"value", file1Hash} }},
            },
            {
                {"path", file2Path},
                {"hash", { {"algo", "sha256"}, {"value", file2Hash} }},
                {"script", file2Script}
            },
            {
                {"path", file3Path},
                {"hash", { {"algo", "sha256"}, {"value", file3Hash} }},
                {"script", file3Script}
            }
        }},
        {"release", {
            {"arch", releaseArch},
            {"platform", releasePlatform},
            {"type", releaseType},
            {"version", releaseVersion},
            {"timestamp", releaseTimestamp}
        }}
    };

    fs::create_directories(testLastUpdateRecoveryFile.parent_path());
    writeToFile(testLastUpdateRecoveryFile, manifest.dump());

    devconf->loadPrevManifest();

    const auto& files = devconf->prevManifest().files;
    ASSERT_EQ(files.size(), 3u);

    ASSERT_EQ(files[0].installPath, fs::path(file1Path));
    ASSERT_EQ(files[0].hash.algo, "sha256");
    ASSERT_EQ(files[0].isScript, false);

    ASSERT_EQ(files[1].installPath, fs::path(file2Path));
    ASSERT_EQ(files[1].hash.algo, "sha256");
    ASSERT_EQ(files[1].isScript, true);

    ASSERT_EQ(files[2].installPath, fs::path(file3Path));
    ASSERT_EQ(files[2].hash.algo, "sha256");
    ASSERT_EQ(files[2].isScript, true);

    const auto& release = devconf->prevManifest().release;
    ASSERT_EQ(release.arch, releaseArch);
    ASSERT_EQ(release.platform, releasePlatform);
    ASSERT_EQ(release.type, releaseType);
    ASSERT_EQ(release.version, releaseVersion);
}


TEST_F(DeviceConfigTest, LoadLastUpdatePriorityGivesToRecoveryWay)
{
    init("", testLastUpdateFile);
    auto createManifest = [](const std::string& version) {
        std::string file1Path   = "/opt/myapp/app2";
        std::string file1Hash   = "c93c5405894b0fa7021fc39e32e201111111111111111d2a024adf6a5bf28459";

        std::string file2Path   = "prepare.sh";
        std::string file2Hash   = "b759349b9e633738f4b8047bba51a62035e363f6e16379744a7f7840bda237aa";
        bool        file2Script = true;

        std::string file3Path   = "commit.sh";
        std::string file3Hash   = "857256cbded0a1789906bb0ac93a3ac32bd7ff90746e6964d997a9468938d5ef";
        bool        file3Script = true;

        std::string releaseArch      = "ARMv8";
        std::string releasePlatform  = "Linux";
        std::string releaseType      = "Raspberry Pi 4";
        std::string releaseTimestamp = "2025-11-12T10:23:00Z";

        json manifest = {
            {"files", {
                {
                    {"path", file1Path},
                    {"hash", { {"algo", "sha256"}, {"value", file1Hash} }},
                },
                {
                    {"path", file2Path},
                    {"hash", { {"algo", "sha256"}, {"value", file2Hash} }},
                    {"script", file2Script}
                },
                {
                    {"path", file3Path},
                    {"hash", { {"algo", "sha256"}, {"value", file3Hash} }},
                    {"script", file3Script}
                }
            }},
            {"release", {
                {"arch", releaseArch},
                {"platform", releasePlatform},
                {"type", releaseType},
                {"version", version},
                {"timestamp", releaseTimestamp}
            }}
        };

        return manifest.dump();
    };

    std::string expectedVersion = "0.1.5";

    auto recoveryManifest = createManifest(expectedVersion);
    auto newManifest      = createManifest("0.2.0");

    fs::create_directories(testLastUpdateRecoveryFile.parent_path());
    writeToFile(testLastUpdateRecoveryFile, recoveryManifest);
    writeToFile(testLastUpdateFile, newManifest);

    devconf->loadPrevManifest();

    const auto& release = devconf->prevManifest().release;
    ASSERT_EQ(release.version, expectedVersion);
}


TEST_F(DeviceConfigTest, LoadInvalidNoLastUpdateNoThrows)
{
    init(testDevConfFile, "");

    writeToFile(devConfigFile, "invalid json");

    EXPECT_NO_THROW(devconf->loadPrevManifest());
}

TEST_F(DeviceConfigTest, SaveNewUpdateInfoWritesFileAndUpdatesPrevManifest)
{
    init("", testLastUpdateFile);

    ArtifactManifest manifest;
    manifest.release.arch      = "ARMv8";
    manifest.release.platform  = "Linux";
    manifest.release.type      = "Raspberry Pi 4";
    manifest.release.version   = "1.2.3";
    manifest.files.push_back({
        false,
        "/opt/myapp/app2",
        { "sha256", std::vector<uint8_t>{0xc9,0x3c,0x54,0x05,0x89,0x4b,0x0f,0xa7,0x02,0x1f,0xc3,0x9e,0x32,0xe2,0x05,0xe4,0x0c,0xa0,0xab,0xc0,0x63,0x35,0xfd,0x2a,0x02,0x4a,0xdf,0x6a,0x5b,0xf2,0x84,0x59} }
    });

    manifest.files.push_back({
        true,
        "prepare.sh",
        { "sha256", std::vector<uint8_t>{0xb7,0x59,0x34,0x9b,0x9e,0x63,0x37,0x38,0xf4,0xb8,0x04,0x7b,0xba,0x51,0xa6,0x20,0x35,0xe3,0x63,0xf6,0xe1,0x63,0x79,0x74,0x4a,0x7f,0x78,0x40,0xbd,0xa2,0x37,0xaa} }
    });

    devconf->saveNewUpdateInfo(manifest);

    ASSERT_TRUE(fs::exists(testLastUpdateFile));

    std::ifstream infile(testLastUpdateFile, std::ios::binary);

    ASSERT_TRUE(infile.is_open());

    json loadedJson;
    infile >> loadedJson;
    json expectedJson = manifest.saveInJson();

    ASSERT_EQ(loadedJson.dump(), expectedJson.dump());

    const auto& prev = devconf->prevManifest();

    ASSERT_EQ(prev.release.version,           manifest.release.version);
    ASSERT_EQ(prev.files.size(),                                    2u);
    ASSERT_EQ(prev.files[0].installPath, manifest.files[0].installPath);
    ASSERT_EQ(prev.files[0].isScript,       manifest.files[0].isScript);
    ASSERT_EQ(prev.files[1].installPath, manifest.files[1].installPath);
    ASSERT_EQ(prev.files[1].isScript,       manifest.files[1].isScript);
}


TEST_F(DeviceConfigTest, SaveIdWritesToFileAndUpdatesConfig)
{
    init(testDevConfFile, testLastUpdateFile);

    std::string type                   = "Rock Pi";
    std::string platform               = "QNX";
    std::string arch                   = "ARMv9";
    int         pollingIntervalMinutes = 15;
    std::string serverUrl              = "https://example.com";
    int         serverPort             = 8080;
    fs::path    certPath               = "/tmp/cert.pem";
    fs::path    keyPath                = "/tmp/key.pem";
    fs::path    caCertPath             = "/tmp/ca.pem";
    fs::path    publicKeyPath          = "/tmp/pub.pem";

    json content = {
        {"type", type},
        {"platform", platform},
        {"arch", arch},
        {"updatePollingIntervalMinutes", pollingIntervalMinutes},
        {"serverURL", serverUrl},
        {"serverPort", serverPort},
        {"certPath", certPath.string()},
        {"keyPath", keyPath.string()},
        {"caCertPath", caCertPath.string()},
        {"publicKeyPath", publicKeyPath.string()},
    };

    writeToFile(devConfigFile, content.dump(4));

    devconf->loadConfig();
    ASSERT_EQ(devconf->id(), 0u) << "Initial id should be 0";

    uint32_t newId = 12345;
    devconf->saveId(newId);

    ASSERT_EQ(devconf->id(), newId) << "Ids are not equal";

    std::ifstream infile(devConfigFile);
    ASSERT_TRUE(infile.is_open());

    json loadedJson;
    infile >> loadedJson;
    ASSERT_EQ(loadedJson["id"].get<uint32_t>(), newId);

    ASSERT_EQ(loadedJson["type"].get<std::string>(), type);
    ASSERT_EQ(loadedJson["platform"].get<std::string>(), platform);
    ASSERT_EQ(loadedJson["arch"].get<std::string>(), arch);
    ASSERT_EQ(loadedJson["updatePollingIntervalMinutes"].get<int>(), pollingIntervalMinutes);
    ASSERT_EQ(loadedJson["serverURL"].get<std::string>(), serverUrl);
    ASSERT_EQ(loadedJson["serverPort"].get<int>(), serverPort);
    ASSERT_EQ(loadedJson["certPath"].get<fs::path>(), certPath);
    ASSERT_EQ(loadedJson["keyPath"].get<fs::path>(), keyPath);
    ASSERT_EQ(loadedJson["caCertPath"].get<fs::path>(), caCertPath);
    ASSERT_EQ(loadedJson["publicKeyPath"].get<fs::path>(), publicKeyPath);
}

