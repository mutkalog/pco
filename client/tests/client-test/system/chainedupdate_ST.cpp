#include "archivetoolsadapter.h"
#include "core/deviceconfig.h"
#include "sslutilsadapter.h"
#include "systemfixturebase.h"

const int MAX_WAIT_MS = 10000;

class ChainedUpdatesSystemTest : public SystemFixtureBase
{
protected:
    std::atomic<int> nextDeviceId{100};
    fs::path archivePath;
    std::vector<std::string> manifestStrings;
    std::vector<std::string> signatureStrings;
    std::vector<std::vector<uint8_t>> archives;
    std::atomic<bool> successReportReceived{false};
    std::vector<std::string> versions = {"1.2.0", "1.3.0", "1.4.0"};
    std::atomic<int> stage{0};

    void SetUp() override
    {
        SystemFixtureBase::SetUp();

        auto hashOf = [&](const fs::path &p) {
            return SSLUtilsAdapter().sha256FromFile(p.string());
        };

        manifestStrings.resize(versions.size());
        signatureStrings.resize(versions.size());
        archives.resize(versions.size());

        for (size_t i = 0; i < versions.size(); ++i)
        {
            fs::path newDir = BASE_TEST_DIR / ("new_version_" + versions[i]);
            fs::create_directories(newDir);
            writeFile(newDir / "app2", std::string("APP2_BINARY_NEW_") + versions[i]);
            writeFile(newDir / "app3", std::string("APP3_BINARY_NEW_") + versions[i]);
            writeFile(newDir / "prepare.sh", std::string("#!/bin/bash\necho prepare_") + versions[i]);
            writeFile(newDir / "commit.sh", std::string("#!/bin/bash\necho commit_") + versions[i]);

            json manifestJson =
            {
                { "release", {
                    { "version",   versions[i] },
                    { "type",      "Raspberry Pi 4" },
                    { "timestamp", "2025-11-12T10:23:00Z" },
                    { "platform",  "Linux" },
                    { "arch",      "ARMv8" }
                }},
                { "files", json::array({
                    {
                        { "path", "/opt/myapp/app2" },
                        { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(newDir / "app2")) }}}
                    },
                    {
                        { "path", "/opt/myapp/app3" },
                        { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(newDir / "app3")) }}}
                    },
                    {
                        { "script", true },
                        { "path", "prepare.sh" },
                        { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(newDir / "prepare.sh")) }}}
                    },
                    {
                        { "script", true },
                        { "path", "commit.sh" },
                        { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(newDir / "commit.sh")) }}}
                    }
                })}
            };

            manifestStrings[i] = manifestJson.dump();

            fs::path manifestFile = BASE_TEST_DIR / ("manifest_" + versions[i] + ".json");
            fs::path sigFile      = BASE_TEST_DIR / ("manifest_" + versions[i] + ".sig");
            writeFile(manifestFile, manifestStrings[i]);

            std::string signCmd =
                std::string("openssl dgst -sha256 -sign ") +
                PROJECT_ROOT_DIR +
                "/client/sim/security/private.pem -out " +
                sigFile.string() + " " + manifestFile.string();

            if (system(signCmd.c_str()) != 0)
                throw std::runtime_error("Failed to sign manifest via OpenSSL");

            std::string signatureData = readFileBinary(sigFile);
            signatureStrings[i] = json{
                {"signature", {
                    {"algo", "rsa-sha256"},
                    {"keyname", "main-signing-key"},
                    {"value", SSLUtilsAdapter().encodeBase64(std::vector<uint8_t>(signatureData.begin(), signatureData.end()))}
                }}
            }.dump();

            std::vector<std::string> paths =
            {
                (newDir / "app2").string(),
                (newDir / "app3").string(),
                (newDir / "prepare.sh").string(),
                (newDir / "commit.sh").string()
            };

            ASSERT_EQ(ArchiveToolsAdapter().create_archive_from_paths(paths, archives[i]), ARCHIVE_OK);
        }

        server->Post("/register", [this](const httplib::Request& req, httplib::Response& res) {
            json response = {{"status", "registered"}, {"id", nextDeviceId++}};
            res.set_content(response.dump(), "application/json");
            res.status = 200;
        });

        server->Get("/manifest", [this](const httplib::Request&, httplib::Response& res) {
            int idx = stage.load();
            if (idx < static_cast<int>(manifestStrings.size()))
            {
                json response = { {"manifest", manifestStrings[idx]}, {"signature", signatureStrings[idx]} };
                res.set_content(response.dump(), "application/json");
                res.status = httplib::StatusCode::OK_200;
            }
        });

        server->Get("/download", [this](const httplib::Request&, httplib::Response& res) {
            int idx = stage.load();
            if (idx < static_cast<int>(archives.size()))
            {
                res.set_content(reinterpret_cast<const char*>(archives[idx].data()),
                                archives[idx].size(), "application/octet-stream");
                res.status = httplib::StatusCode::OK_200;
            }
        });

        server->Post("/report", [this](const httplib::Request& req, httplib::Response& res) {
            auto j = json::parse(req.body);
            int currentStage = stage.load();
            if (currentStage < static_cast<int>(versions.size())) {
                if (j["status"] == "SUCCESS" && j["current_version"] == versions[currentStage]) {
                    stage.fetch_add(1);
                    if (stage.load() == static_cast<int>(versions.size()))
                        successReportReceived = true;
                }
            }

            res.status = httplib::StatusCode::OK_200;
        });

        startServer();

        buildDockerImage(fs::path(PROJECT_ROOT_DIR) / "client/sim");
        runDockerContainer();
    }
};


TEST_F(ChainedUpdatesSystemTest, ExecuteThreeUpdatesInSequence)
{
    int waited = 0;
    while (!successReportReceived && waited < MAX_WAIT_MS)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waited += 100;
    }

    ASSERT_EQ(stage, 3);
}
