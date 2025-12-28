#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "core/artifactmanifest.h"

namespace {
const fs::path testDevConfFile            = "/tmp/devconf.json";
const fs::path testLastUpdateFile         = "/tmp/last-update.json";
const fs::path testLastUpdateRecoveryFile = testLastUpdateFile.parent_path() / "rollback" / testLastUpdateFile.filename();
const fs::path badDevConfFile             = "/fakedir/devconf.json";
const fs::path badLastUpdateFile          = "/fakedir/last-update.json";

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

class ArtifactManifestTest : public ArtifactManifest
{
public:
    using ArtifactManifest::rawHashFromString;
    using ArtifactManifest::stringHashFromRaw;
};


TEST(ArtifactManifestTest, RawHashFromStringSuccess)
{
    ArtifactManifestTest manifest;

    std::string hex = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";
    auto raw = manifest.rawHashFromString(hex);

    ASSERT_EQ(raw.size(), hex.size() / 2);

    std::vector<uint8_t> expected = {
        0xc9, 0x3c, 0x54, 0x05, 0x89, 0x4b, 0x0f, 0xa7,
        0x02, 0x1f, 0xc3, 0x9e, 0x32, 0xe2, 0x05, 0xe4,
        0x0c, 0xa0, 0xab, 0xc0, 0x63, 0x35, 0xfd, 0x2a,
        0x02, 0x4a, 0xdf, 0x6a, 0x5b, 0xf2, 0x84, 0x59
    };

    ASSERT_EQ(raw, expected);
}

TEST(ArtifactManifestTest, RawHashFromStringReturnsEmptyVector)
{
    ArtifactManifestTest manifest;
    std::string hex = "";
    auto raw = manifest.rawHashFromString(hex);

    ASSERT_TRUE(raw.empty());
}

TEST(ArtifactManifestTest, RawHashFromStringInvalidHexCharactersThrows)
{
    ArtifactManifestTest manifest;
    std::string hex = "gh";

    ASSERT_THROW(manifest.rawHashFromString(hex), std::invalid_argument);
}

TEST(ArtifactManifestTest, StringHashFromRawSuccess)
{
    ArtifactManifestTest manifest;

    std::vector<uint8_t> raw = {
        0xc9, 0x3c, 0x54, 0x05, 0x89, 0x4b, 0x0f, 0xa7,
        0x02, 0x1f, 0xc3, 0x9e, 0x32, 0xe2, 0x05, 0xe4,
        0x0c, 0xa0, 0xab, 0xc0, 0x63, 0x35, 0xfd, 0x2a,
        0x02, 0x4a, 0xdf, 0x6a, 0x5b, 0xf2, 0x84, 0x59
    };

    std::string expected = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";

    auto hex = manifest.stringHashFromRaw(raw);

    ASSERT_EQ(hex, expected);
}

TEST(ArtifactManifestTest, StringHashFromRawEmptyVectorReturnsEmptyString)
{
    ArtifactManifestTest manifest;

    std::vector<uint8_t> raw;
    std::string hex = manifest.stringHashFromRaw(raw);

    ASSERT_TRUE(hex.empty());
}

TEST(ArtifactManifestTest, RawAndStringConversionInverse)
{
    ArtifactManifestTest manifest;

    std::string originalHex = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";

    auto raw = manifest.rawHashFromString(originalHex);
    auto hex = manifest.stringHashFromRaw(raw);

    ASSERT_EQ(hex, originalHex);
}

TEST(ArtifactManifestTest, LoadFromJsonParsesCorrectly)
{
    ArtifactManifestTest manifest;

    json manifestJson = {
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

    manifest.loadFromJson(manifestJson);

    ASSERT_EQ(manifest.release.arch, releaseArch);
    ASSERT_EQ(manifest.release.platform, releasePlatform);
    ASSERT_EQ(manifest.release.type, releaseType);
    ASSERT_EQ(manifest.release.version, releaseVersion);

    std::ostringstream ss;
    ss << std::put_time(&manifest.release.timestamp, "%Y-%m-%dT%H:%M:%SZ");
    ASSERT_EQ(ss.str(), releaseTimestamp);

    ASSERT_EQ(manifest.files.size(), 3u);

    ASSERT_EQ(manifest.files[0].installPath.string(), file1Path);
    ASSERT_EQ(manifest.files[0].hash.algo, "sha256");
    ASSERT_EQ(manifest.stringHashFromRaw(manifest.files[0].hash.value), file1Hash);
    ASSERT_FALSE(manifest.files[0].isScript);

    ASSERT_EQ(manifest.files[1].installPath.string(), file2Path);
    ASSERT_EQ(manifest.files[1].hash.algo, "sha256");
    ASSERT_EQ(manifest.stringHashFromRaw(manifest.files[1].hash.value), file2Hash);
    ASSERT_TRUE(manifest.files[1].isScript);

    ASSERT_EQ(manifest.files[2].installPath.string(), file3Path);
    ASSERT_EQ(manifest.files[2].hash.algo, "sha256");
    ASSERT_EQ(manifest.stringHashFromRaw(manifest.files[2].hash.value), file3Hash);
    ASSERT_TRUE(manifest.files[2].isScript);
}


TEST(ArtifactManifestTest, SaveInJsonSerializesCorrectly) {
    ArtifactManifestTest manifest;

    std::tm tm = {};
    std::istringstream ss(releaseTimestamp);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    manifest.release = {releaseVersion, releaseType, releasePlatform, releaseArch, tm};

    ArtifactManifest::File f1;
    f1.isScript    = false;
    f1.installPath = file1Path;
    f1.hash.algo   = "sha256";
    f1.hash.value  = manifest.rawHashFromString(file1Hash);
    manifest.files.push_back(f1);

    ArtifactManifest::File f2;
    f2.isScript    = file2Script;
    f2.installPath = file2Path;
    f2.hash.algo   = "sha256";
    f2.hash.value  = manifest.rawHashFromString(file2Hash);
    manifest.files.push_back(f2);

    ArtifactManifest::File f3;
    f3.isScript    = file3Script;
    f3.installPath = file3Path;
    f3.hash.algo   = "sha256";
    f3.hash.value  = manifest.rawHashFromString(file3Hash);
    manifest.files.push_back(f3);

    json data = manifest.saveInJson();

    ASSERT_EQ(data["release"]["version"], releaseVersion);
    ASSERT_EQ(data["release"]["type"], releaseType);
    ASSERT_EQ(data["release"]["platform"], releasePlatform);
    ASSERT_EQ(data["release"]["arch"], releaseArch);
    ASSERT_EQ(data["release"]["timestamp"], releaseTimestamp);

    ASSERT_EQ(data["files"].size(), 3u);

    ASSERT_EQ(data["files"][0]["path"], file1Path);
    ASSERT_EQ(data["files"][0]["script"], false);
    ASSERT_EQ(data["files"][0]["hash"]["algo"], "sha256");
    ASSERT_EQ(data["files"][0]["hash"]["value"], file1Hash);

    ASSERT_EQ(data["files"][1]["path"], file2Path);
    ASSERT_EQ(data["files"][1]["script"], file2Script);
    ASSERT_EQ(data["files"][1]["hash"]["algo"], "sha256");
    ASSERT_EQ(data["files"][1]["hash"]["value"], file2Hash);

    ASSERT_EQ(data["files"][2]["path"], file3Path);
    ASSERT_EQ(data["files"][2]["script"], file3Script);
    ASSERT_EQ(data["files"][2]["hash"]["algo"], "sha256");
    ASSERT_EQ(data["files"][2]["hash"]["value"], file3Hash);
}


TEST(ArtifactManifestTest, ClearResetsAllFields)
{
    ArtifactManifestTest manifest;

    std::tm tm = {};
    std::istringstream ss(releaseTimestamp);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    manifest.release = {releaseVersion, releaseType, releasePlatform, releaseArch, tm};

    ArtifactManifest::File f1;
    f1.isScript    = false;
    f1.installPath = file1Path;
    f1.hash.algo   = "sha256";
    f1.hash.value  = manifest.rawHashFromString(file1Hash);
    manifest.files.push_back(f1);

    ArtifactManifest::File f2;
    f2.isScript    = file2Script;
    f2.installPath = file2Path;
    f2.hash.algo   = "sha256";
    f2.hash.value  = manifest.rawHashFromString(file2Hash);
    manifest.files.push_back(f2);

    ASSERT_FALSE(manifest.release.version.empty());
    ASSERT_FALSE(manifest.files.empty());

    manifest.clear();

    ASSERT_TRUE(manifest.release.version.empty());
    ASSERT_TRUE(manifest.release.type.empty());
    ASSERT_TRUE(manifest.release.platform.empty());
    ASSERT_TRUE(manifest.release.arch.empty());

    std::tm zeroTm{};
    ASSERT_EQ(std::memcmp(&manifest.release.timestamp, &zeroTm, sizeof(tm)), 0);

    ASSERT_TRUE(manifest.files.empty());
}
