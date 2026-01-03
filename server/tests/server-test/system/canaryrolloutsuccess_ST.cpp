#include <archivetools.h>
#include "systemfixturebase.h"


class CanaryRolloutSuccessSystemTest : public SystemFixtureBase
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

TEST_F(CanaryRolloutSuccessSystemTest, CanaryToStableFullProgression)
{
    const int TOTAL_DEVICES = 10;
    const std::string version = "3.0.0";
    std::vector<int> deviceIds;

    for (int i = 0; i < TOTAL_DEVICES; ++i)
    {
        json body =
        {
            {"type", "Raspberry Pi 4"}, {"arch", "ARMv8"},
            {"platform", "Linux"}, {"pollingInterval", 1}
        };
        auto res = client->Post(REGISTER_PATH.c_str(), body.dump(), "application/json");
        ASSERT_EQ(res->status, 200);
        deviceIds.push_back(json::parse(res->body)["id"].get<int>());
    }

    fs::path archiveDir = BASE_TEST_DIR / "canary_archive_src";
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

    fs::path tmpdir = BASE_TEST_DIR / "canary_upload_tmp";
    fs::create_directories(tmpdir);
    fs::path mFile = tmpdir / "manifest.json";
    writeFile(mFile, innerManifest);
    std::string sig = signAndBase64(mFile, tmpdir);
    json sigJson =
    {
        {"signature", {{"algo", "rsa-sha256"}, {"keyname", "main"}, {"value", sig}}}
    };

    auto resUpload = postMultipartUpload(UPLOAD_PATH, innerManifest, sigJson.dump(), archiveData,
                                         {{"canary","true"}, {"percentage","20"}});
    ASSERT_EQ(resUpload->status, 200);

    std::set<int> updatedDevices;
    int iterations = 0;

    while (updatedDevices.size() < TOTAL_DEVICES && iterations < 10)
    {
        iterations++;
        std::vector<int> currentBatch;

        for (int id : deviceIds)
        {
            if (updatedDevices.count(id))
                continue;

            std::string q = "?id=" + std::to_string(id) +
                            "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                            "&platform=Linux&arch=ARMv8";
            auto res = client->Get((MANIFEST_PATH + q).c_str());

            if (res && res->status == 200)
            {
                currentBatch.push_back(id);
            }
        }

        for (int id : currentBatch)
        {
            std::string q = "?id=" + std::to_string(id) + "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                            "&platform=Linux&arch=ARMv8";

            auto resDown = client->Get((DOWNLOAD_PATH + q).c_str());
            ASSERT_EQ(resDown->status, 200);

            bool ok = sendDeviceReport(id, "Raspberry Pi 4", "ARMv8", "Linux", "SUCCESS", 0, version);
            ASSERT_TRUE(ok);

            updatedDevices.insert(id);
        }

        std::this_thread::sleep_for(std::chrono::seconds(12));
    }

    ASSERT_EQ(updatedDevices.size(), TOTAL_DEVICES) << "Rollout failed to reach 100% in reasonable time";

    for (int id : deviceIds)
    {
        std::string q = "?id=" + std::to_string(id) + "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                        "&platform=Linux&arch=ARMv8";
        auto res = client->Get((MANIFEST_PATH + q).c_str());
        ASSERT_EQ(res->status, 200) << "Stable fallback should work for device " << id;

        auto j = json::parse(res->body);
        ASSERT_EQ(json::parse(j["manifest"].get<std::string>())["release"]["version"], version);
    }

    {
        pqxx::connection conn("dbname=pco_test user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);

        pqxx::result res = txn.exec("SELECT is_canary, active FROM releases WHERE version = " + txn.quote(version));

        ASSERT_EQ(res.size(), 1);
        ASSERT_FALSE(res[0]["is_canary"].as<bool>());
        ASSERT_TRUE(res[0]["active"].as<bool>());

        txn.commit();
    }
}
