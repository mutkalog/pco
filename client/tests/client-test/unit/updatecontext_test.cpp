#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "core/updatecontext.h"
#include "mocks/deviceinfo_mock.h"

namespace {
const fs::path testDevConfFile            = "/tmp/devconf.json";
const fs::path testLastUpdateFile         = "/tmp/last-update.json";
const fs::path testLastUpdateRecoveryFile = testLastUpdateFile.parent_path() / "rollback" / testLastUpdateFile.filename();
const fs::path badDevConfFile             = "/fakedir/devconf.json";
const fs::path badLastUpdateFile          = "/fakedir/last-update.json";

const std::string file0Path        = "/opt/myapp/app1";
const std::string file0Hash        = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";
const std::string file1Path        = "/opt/myapp/app2";
const std::string file1Hash        = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";
const std::string file2Path        = "prepare.sh";
const std::string file2Hash        = "b759349b9e633738f4b8047bba51a62035e363f6e16379744a7f7840bda237aa";
const bool        file2Script      = true;
const std::string file3Path        = "commit.sh";
const std::string file3Hash        = "857256cbded0a1789906bb0ac93a3ac32bd7ff90746e6964d997a9468938d5ef";
const bool        file3Script      = true;
const std::string releaseArch      = "ARMv8";
const std::string releasePlatform  = "Linux";
const std::string releaseType      = "Raspberry Pi 4";
const std::string releaseVersion   = "1.2.1";
const std::string releaseTimestamp = "2025-11-12T10:23:00Z";
}


using json = nlohmann::ordered_json;
using ::testing::NiceMock;


TEST(UpdateContext, updateEnvironmentVarsSuccess)
{
    auto mockDevConf = std::make_unique<NiceMock<MockClientConfig>>();

    ArtifactManifest manifest;
    ArtifactManifest::File f0; f0.isScript = false; f0.installPath = file0Path; manifest.files.push_back(f0);
    ArtifactManifest::File f1; f1.isScript = false; f1.installPath = file1Path; manifest.files.push_back(f1);
    ArtifactManifest::File f2; f2.isScript = true;  f2.installPath = file2Path; manifest.files.push_back(f2);
    ArtifactManifest::File f3; f3.isScript = true;  f3.installPath = file3Path; manifest.files.push_back(f3);

    EXPECT_CALL(*mockDevConf, prevManifest())
        .WillOnce(testing::Return(manifest));

    UpdateContext ctx(std::move(mockDevConf), nullptr, nullptr, nullptr, nullptr, "", "");

    ctx.updateEnvironmentVars();

    const char* envVal = std::getenv("PCO_ROLLBACK_ARTIFACTS_PATHS");
    ASSERT_NE(envVal, nullptr);

    std::string expected = std::string(file0Path) + ":" + std::string(file1Path);
    ASSERT_EQ(std::string(envVal), expected);

    EXPECT_EQ(unsetenv("PCO_ROLLBACK_ARTIFACTS_PATHS"), 0);
}


TEST(UpdateContext, updateEnvironmentVarsEmptyVar)
{
    auto mockDevConf = std::make_unique<NiceMock<MockClientConfig>>();

    ArtifactManifest manifest;
    ArtifactManifest::File f2; f2.isScript = true; f2.installPath = file2Path; manifest.files.push_back(f2);
    ArtifactManifest::File f3; f3.isScript = true; f3.installPath = file3Path; manifest.files.push_back(f3);

    EXPECT_CALL(*mockDevConf, prevManifest()).WillOnce(testing::Return(manifest));
    UpdateContext ctx(std::move(mockDevConf), nullptr, nullptr, nullptr, nullptr, "", "");

    ctx.updateEnvironmentVars();

    const char* envVal = std::getenv("PCO_ROLLBACK_ARTIFACTS_PATHS");
    ASSERT_NE(envVal, nullptr);

    std::string expected = "";
    ASSERT_EQ(std::string(envVal), expected);

    EXPECT_EQ(unsetenv("PCO_ROLLBACK_ARTIFACTS_PATHS"), 0);
}


TEST(UpdateContext, DumpContextSuccess)
{
    UpdateContext ctx(nullptr, nullptr, nullptr, nullptr, nullptr, "", "");

    ctx.rollback      = true;
    ctx.recovering    = false;
    ctx.reportMessage = {42, "ok"};
    ctx.busyResources = BusyResources{1, 0, 1};

    ArtifactManifest manifest;
    manifest.release.arch      = releaseArch;
    manifest.release.platform  = releasePlatform;
    manifest.release.type      = releaseType;
    manifest.release.version   = releaseVersion;
    manifest.files.push_back({
        false,
        file1Path,
        { "sha256", std::vector<uint8_t>{0xc9,0x3c,0x54,0x05,0x89,0x4b,0x0f,0xa7,0x02,0x1f,0xc3,0x9e,0x32,0xe2,0x05,0xe4,0x0c,0xa0,0xab,0xc0,0x63,0x35,0xfd,0x2a,0x02,0x4a,0xdf,0x6a,0x5b,0xf2,0x84,0x59} }
    });

    manifest.files.push_back({
        true,
        "prepare.sh",
        { "sha256", std::vector<uint8_t>{0xb7,0x59,0x34,0x9b,0x9e,0x63,0x37,0x38,0xf4,0xb8,0x04,0x7b,0xba,0x51,0xa6,0x20,0x35,0xe3,0x63,0xf6,0xe1,0x63,0x79,0x74,0x4a,0x7f,0x78,0x40,0xbd,0xa2,0x37,0xaa} }
    });
    ctx.manifest = std::move(manifest);

    std::istringstream ss(releaseTimestamp);
    ss >> std::get_time(&ctx.manifest.release.timestamp, "%Y-%m-%dT%H:%M:%SZ");


    json result = ctx.dumpContext();


    ASSERT_TRUE(result["rollback"]);
    ASSERT_FALSE(result["recovering"]);

    ASSERT_EQ(result["reportMessage"][0], 42);
    ASSERT_EQ(result["reportMessage"][1], "ok");

    uint32_t expectedBusy;
    std::memcpy(&expectedBusy, &ctx.busyResources, sizeof(BusyResources));
    ASSERT_EQ(result["busyResources"], expectedBusy);

    ASSERT_EQ(result["manifest"]["release"]["arch"], releaseArch);
    ASSERT_EQ(result["manifest"]["release"]["platform"], releasePlatform);
    ASSERT_EQ(result["manifest"]["release"]["type"], releaseType);
    ASSERT_EQ(result["manifest"]["release"]["version"], releaseVersion);

    ASSERT_EQ(result["manifest"]["files"].size(), 2);
    ASSERT_EQ(result["manifest"]["files"][0]["path"], file1Path);
    ASSERT_EQ(result["manifest"]["files"][1]["script"], true);
}


TEST(UpdateContext, LoadContextSuccess)
{
    UpdateContext ctx(nullptr, nullptr, nullptr, nullptr, nullptr, "", "");

    json input;

    input["rollback"]   = true;
    input["recovering"] = false;
    input["reportMessage"] = {42, "ok"};

    BusyResources expectedBusy{1, 0, 0};
    uint32_t expectedBusyWord;
    std::memcpy(&expectedBusyWord, &expectedBusy, sizeof(BusyResources));
    input["busyResources"] = expectedBusyWord;

    input["pathToRollbackPathMap"] = {
        { file1Path, "/rollback/app1" },
        { file2Path, "/rollback/app2" }
    };

    input["manifest"] = {
        {"release", {
            {"arch",     releaseArch},
            {"platform", releasePlatform},
            {"type",     releaseType},
            {"version",  releaseVersion},
            {"timestamp", releaseTimestamp}
        }},
        {"files", {
            {
                {"path", file1Path},
                {"script", false},
                {"hash", {
                    {"algo", "sha256"},
                    {"value", file1Hash}
                }}
            },
            {
                {"path", file2Path},
                {"script", true},
                {"hash", {
                    {"algo", "sha256"},
                    {"value", file2Hash}
                }}
            }
        }}
    };

    ctx.loadContext(input);

    ASSERT_TRUE(ctx.rollback);
    ASSERT_FALSE(ctx.recovering);

    ASSERT_EQ(ctx.reportMessage.first, 42);
    ASSERT_EQ(ctx.reportMessage.second, "ok");

    BusyResources actualBusy;
    uint32_t actualBusyWord;
    std::memcpy(&actualBusyWord, &ctx.busyResources, sizeof(BusyResources));
    ASSERT_EQ(actualBusyWord, expectedBusyWord);

    ASSERT_EQ(ctx.pathToRollbackPathMap.size(), 2);
    ASSERT_EQ(ctx.pathToRollbackPathMap[file1Path], "/rollback/app1");
    ASSERT_EQ(ctx.pathToRollbackPathMap[file2Path], "/rollback/app2");

    ASSERT_EQ(ctx.manifest.release.arch, releaseArch);
    ASSERT_EQ(ctx.manifest.release.platform, releasePlatform);
    ASSERT_EQ(ctx.manifest.release.type, releaseType);
    ASSERT_EQ(ctx.manifest.release.version, releaseVersion);

    std::tm tm{};
    std::istringstream ss(releaseTimestamp);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    ASSERT_EQ(std::mktime(&ctx.manifest.release.timestamp),
              std::mktime(&tm));

    ASSERT_EQ(ctx.manifest.files.size(), 2);

    ASSERT_EQ(ctx.manifest.files[0].installPath, file1Path);
    ASSERT_FALSE(ctx.manifest.files[0].isScript);

    ASSERT_EQ(ctx.manifest.files[1].installPath, file2Path);
    ASSERT_TRUE(ctx.manifest.files[1].isScript);
}

