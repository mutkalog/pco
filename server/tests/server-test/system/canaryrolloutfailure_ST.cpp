#include <archivetools.h>
#include "systemfixturebase.h"

class CanaryRolloutFailureSystemTest : public SystemFixtureBase
{
protected:
    const int CANARY_DEVICE_COUNT = 10;
    void SetUp() override
    {
        SystemFixtureBase::SetUp();
        buildDockerImage(fs::path(PROJECT_ROOT_DIR) / "server" / "sim");
        runDockerContainer();
        waitForServerUp(5000);
    }
};

TEST_F(CanaryRolloutFailureSystemTest, AutomatedRollbackOnFailure)
{
    const std::string version = "4.0.0";
    std::vector<int> deviceIds;

    for (int i = 0; i < CANARY_DEVICE_COUNT; ++i)
    {
        json body =
        {
            {"type", "Raspberry Pi 4"}, {"arch", "ARMv8"},
            {"platform", "Linux"}, {"pollingInterval", 1}
        };
        auto res = client->Post(REGISTER_PATH.c_str(), body.dump(), "application/json");
        ASSERT_EQ(res->status, 200);
        deviceIds.push_back(json::parse(res->body)["id"].get<int>());

        int id = json::parse(res->body)["id"].get<int>();
        deviceIds.push_back(id);
        std::string q = "?id=" + std::to_string(id) +
                        "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                        "&platform=Linux&arch=ARMv8";
        client->Get((MANIFEST_PATH + q).c_str());
    }

    fs::path archiveDir = BASE_TEST_DIR / "rollback_archive_src";
    fs::remove_all(archiveDir);
    fs::create_directories(archiveDir);
    writeFile(archiveDir / "app_bin", "PAYLOAD");

    std::vector<uint8_t> archiveData;
    ASSERT_EQ(ArchiveTools::create_archive_from_paths({(archiveDir / "app_bin").string()}, archiveData), 0);

    std::string innerManifest = R"({
        "release": {
            "version": ")" + version + R"(",
            "type": "Raspberry Pi 4",
            "timestamp": "2025-11-12T10:23:00Z",
            "platform": "Linux",
            "arch": "ARMv8"
        },
        "files": [{"path": "/opt/app", "hash": {"algo":"sha256", "value":"v"}}]
    })";

    fs::path tmpdir = BASE_TEST_DIR / "rollback_upload_tmp";
    fs::create_directories(tmpdir);
    fs::path mFile = tmpdir / "manifest.json";
    writeFile(mFile, innerManifest);
    std::string sig = signAndBase64(mFile, tmpdir);
    json sigJson =
    {
        {"signature", {{"algo", "rsa-sha256"}, {"keyname", "main"}, {"value", sig}}}
    };

    auto resUpload = postMultipartUpload(UPLOAD_PATH, innerManifest, sigJson.dump(), archiveData,
                                         {{"canary","true"}, {"percentage","50"}});
    ASSERT_EQ(resUpload->status, 200);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    int targetDeviceId = -1;
    for (int id : deviceIds)
    {
        std::string q = "?id=" + std::to_string(id) +
                        "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                        "&platform=Linux&arch=ARMv8";
        auto res = client->Get((MANIFEST_PATH + q).c_str());
        if (res && res->status == 200)
        {
            targetDeviceId = id;
            break;
        }
    }

    ASSERT_NE(targetDeviceId, -1);

    std::string qDown = "?id=" + std::to_string(targetDeviceId) +
                        "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                        "&platform=Linux&arch=ARMv8";
    auto resDown = client->Get((DOWNLOAD_PATH + qDown).c_str());
    ASSERT_EQ(resDown->status, 200);

    bool ok = sendDeviceReport(targetDeviceId, "Raspberry Pi 4", "ARMv8", "Linux", "FAILED", 1, version);
    ASSERT_TRUE(ok);

    std::this_thread::sleep_for(std::chrono::seconds(25));

    int otherDeviceId = -1;
    for (int id : deviceIds)
    {
        if (id == targetDeviceId)
            continue;
        otherDeviceId = id;
        break;
    }

    std::string qOther = "?id=" + std::to_string(otherDeviceId) +
                         "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                         "&platform=Linux&arch=ARMv8";
    auto resFinal = client->Get((MANIFEST_PATH + qOther).c_str());
    ASSERT_EQ(resFinal->status, 400);

    {
        pqxx::connection conn("dbname=pco_test user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);

        pqxx::result res = txn.exec("SELECT is_canary, active FROM releases WHERE version = " + txn.quote(version));

        ASSERT_EQ(res.size(), 1);
        ASSERT_FALSE(res[0]["is_canary"].as<bool>());
        ASSERT_FALSE(res[0]["active"].as<bool>());

        txn.commit();
    }
}
