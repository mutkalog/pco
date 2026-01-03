#include <archivetools.h>
#include "systemfixturebase.h"


class FullCycleSystemTest : public SystemFixtureBase
{
protected:
    const int DEVICE_COUNT = 5;

    void SetUp() override
    {
        SystemFixtureBase::SetUp();
        buildDockerImage(fs::path(PROJECT_ROOT_DIR) / "server" / "sim");
        runDockerContainer();
        waitForServerUp(5000);
    }
};

TEST_F(FullCycleSystemTest, FullReleaseHappyPath)
{
    std::vector<int> deviceIds;
    for (int i = 0; i < DEVICE_COUNT; ++i)
    {
        json body = {
            {"type", "Raspberry Pi 4"},
            {"arch", "ARMv8"},
            {"platform", "Linux"},
            {"pollingInterval", 10}
        };

        ASSERT_TRUE(client->is_valid());

        auto res = client->Post(REGISTER_PATH.c_str(), body.dump(), "application/json");
        ASSERT_TRUE(res);
        ASSERT_EQ(res->status, 200);

        auto j = json::parse(res->body);
        ASSERT_TRUE(j.contains("id"));
        deviceIds.push_back(j["id"].get<int>());
    }

    const std::string version = "2.0.0";
    std::string innerManifest = R"({
        "release": {
            "version": ")" + version + R"(",
            "type": "Raspberry Pi 4",
            "timestamp": "2025-11-12T10:23:00Z",
            "platform": "Linux",
            "arch": "ARMv8"
        },
        "files": [
            {"path": "/opt/myapp/app2", "hash": {"algo":"sha256", "value":"hash-app2"}},
            {"path": "/opt/myapp/app3", "hash": {"algo":"sha256", "value":"hash-app3"}}
        ]
    })";

    fs::path tmpdir = BASE_TEST_DIR / "upload_tmp";
    fs::create_directories(tmpdir);
    fs::path manifestFile = tmpdir / "manifest.json";
    writeFile(manifestFile, innerManifest);

    std::string signatureBase64 = signAndBase64(manifestFile, tmpdir);

    json signatureJson = {
        {"signature", {
            {"algo", "rsa-sha256"},
            {"keyname", "main-signing-key"},
            {"value", signatureBase64}
        }}
    };

    fs::path archiveDir = BASE_TEST_DIR / "archive_src";
    fs::remove_all(archiveDir);
    fs::create_directories(archiveDir);

    fs::path f1 = archiveDir / "app2";
    fs::path f2 = archiveDir / "app3";

    writeFile(f1, "HELLO APP2");
    writeFile(f2, "HELLO APP3");

    std::vector<std::string> paths = { f1.string(), f2.string() };
    std::vector<uint8_t> archiveData;

    int rc = ArchiveTools::create_archive_from_paths(paths, archiveData);
    ASSERT_EQ(rc, 0);
    ASSERT_FALSE(archiveData.empty());

    auto resUpload = postMultipartUpload(UPLOAD_PATH,
                                         innerManifest,
                                         signatureJson.dump(),
                                         archiveData,
                                         {{"canary","false"}});
    ASSERT_TRUE(resUpload);
    ASSERT_EQ(resUpload->status, 200);

    for (int id : deviceIds)
    {
        std::string q = "?id=" + std::to_string(id) +
                        "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                        "&platform=Linux&arch=ARMv8";
        auto res = client->Get((MANIFEST_PATH + q).c_str());
        ASSERT_TRUE(res);
        ASSERT_EQ(res->status, 200);
        auto j = json::parse(res->body);
        ASSERT_TRUE(j.contains("manifest"));
    }

    for (int id : deviceIds)
    {
        std::string q = "?id=" + std::to_string(id) +
                        "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                        "&platform=Linux&arch=ARMv8";
        auto res = client->Get((DOWNLOAD_PATH + q).c_str());
        ASSERT_TRUE(res);
        ASSERT_EQ(res->status, 200);
        ASSERT_FALSE(res->body.empty());
    }

    for (int id : deviceIds)
    {
        bool ok = sendDeviceReport(id, "Raspberry Pi 4", "ARMv8", "Linux", "SUCCESS", 0, version);
        ASSERT_TRUE(ok);
    }
}
