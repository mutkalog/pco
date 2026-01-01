#include "systemfixturebase.h"


class RestartDuringProcessInCommittingStateSystemTest : public SystemFixtureBase
{
protected:
    std::atomic<int> nextDeviceId{200};

    fs::path archivePath;
    std::string manifestString;
    std::vector<uint8_t> archiveData;
    std::atomic<bool> successReportReceived{false};

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();

        fs::create_directories(BASE_TEST_DIR / "opt/myapp");
        writeFile(BASE_TEST_DIR / "opt/myapp/app2", "APP2_BINARY");
        writeFile(BASE_TEST_DIR / "opt/myapp/app3", "APP3_BINARY");
        writeFile(BASE_TEST_DIR / "prepare.sh", "#!/bin/bash\necho prepare");
        writeFile(BASE_TEST_DIR / "commit.sh",  "#!/bin/bash\necho commit");

        createDeviceConfig(CONF_FILE, std::nullopt);
        createStateMachine(BASE_TEST_DIR / "last_update.json", StateExecutor::REGISTRATION);

        auto hashOf = [&](const fs::path& p) {
            return stateMachine->context.cryptoUtils->sha256FromFile(p.string());
        };

        json manifestJson =
        {
            { "release", {
                { "version",   "2.0.0" },
                { "type",      "Raspberry Pi 4" },
                { "timestamp", "2025-11-12T10:23:00Z" },
                { "platform",  "Linux" },
                { "arch",      "ARMv8" }
            }},
            { "files", json::array({
                {
                    { "path", (BASE_TEST_DIR / "opt/myapp/app2").c_str() },
                    { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(BASE_TEST_DIR / "opt/myapp/app2")) }}}
                },
                {
                    { "path", (BASE_TEST_DIR / "opt/myapp/app3").c_str() },
                    { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(BASE_TEST_DIR / "opt/myapp/app3")) }}}
                },
                {
                    { "script", true },
                    { "path", "prepare.sh" },
                    { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(BASE_TEST_DIR / "prepare.sh")) }}}
                },
                {
                    { "script", true },
                    { "path", "commit.sh" },
                    { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(BASE_TEST_DIR / "commit.sh")) }}}
                }
            })}
        };

        manifestString = manifestJson.dump();

        fs::path manifestFile = BASE_TEST_DIR / "manifest.json";
        fs::path sigFile      = BASE_TEST_DIR / "manifest.json.sig";
        writeFile(manifestFile, manifestString);

        std::string signCmd = std::string("openssl dgst -sha256 -sign ") + PROJECT_ROOT_DIR +
                              "/client/tests/resources/security/private.pem -out " +
                              sigFile.string() + " " + manifestFile.string();

        if (system(signCmd.c_str()) != 0) {
            throw std::runtime_error("Failed to sign manifest via OpenSSL");
        }

        std::string signatureData = readFileBinary(sigFile);
        std::string signatureB64  = stateMachine->context.cryptoUtils->encodeBase64(
            std::vector<uint8_t>(signatureData.begin(), signatureData.end())
        );

        json sigJson = {
            {"signature", {
                {"algo", "rsa-sha256"},
                {"keyname", "main-signing-key"},
                {"value", signatureB64}
            }}
        };

        std::string signatureString = sigJson.dump();

        std::vector<std::string> paths =
        {
            (BASE_TEST_DIR / "opt/myapp/app2").string(),
            (BASE_TEST_DIR / "opt/myapp/app3").string(),
            (BASE_TEST_DIR / "prepare.sh").string(),
            (BASE_TEST_DIR / "commit.sh").string()
        };

        ASSERT_EQ(
            stateMachine->context.archiveTools->create_archive_from_paths(paths, archiveData),
            ARCHIVE_OK
        );

        server->Post("/register", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(json{{ "status", "registered" }, { "id", nextDeviceId++ }}.dump(), "application/json");
            res.status = httplib::StatusCode::OK_200;
        });

        server->Get("/manifest", [this, signatureString](const httplib::Request&, httplib::Response& res) {
            json response {
                { "manifest",  manifestString },
                { "signature", signatureString }
            };
            res.set_content(response.dump(), "application/json");
            res.status = httplib::StatusCode::OK_200;
        });

        server->Get("/download", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(reinterpret_cast<const char*>(archiveData.data()), archiveData.size(), "application/octet-stream");
            res.status = httplib::StatusCode::OK_200;
        });

        server->Post("/report", [this](const httplib::Request& req, httplib::Response& res) {
            auto j = json::parse(req.body);
            if (j["status"] == "SUCCESS" && j["current_version"] == "2.0.0") {
                successReportReceived = true;
            }
            res.status = httplib::StatusCode::OK_200;
        });

        startServer();
    }
};


TEST_F(RestartDuringProcessInCommittingStateSystemTest, RestartWhileCommittingResumesAndCompletes)
{
    const int MAX_STEPS_TO_COMMITTING = 8;
    int steps = 0;

    while (stateMachine->state() != StateExecutor::COMMITTING &&
           ++steps < MAX_STEPS_TO_COMMITTING)
    {
        stateMachine->run();
    }

    ASSERT_LT(steps, MAX_STEPS_TO_COMMITTING);
    EXPECT_EQ(stateMachine->state(), StateExecutor::COMMITTING);

    stateMachine.reset();

    createStateMachine(BASE_TEST_DIR / "last_update.json", StateExecutor::REGISTRATION);

    const int MAX_STEPS = 3;
    steps = 0;
    while (stateMachine->state() != StateExecutor::IDLE && ++steps < MAX_STEPS)
    {
        stateMachine->run();
    }

    ASSERT_LT(steps, MAX_STEPS);

    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);

    EXPECT_TRUE(fs::exists(BASE_TEST_DIR / "opt/myapp/app2"));
    EXPECT_TRUE(fs::exists(BASE_TEST_DIR / "opt/myapp/app3"));
    EXPECT_EQ(
        readFileBinary(BASE_TEST_DIR / "opt/myapp/app2"),
        "APP2_BINARY"
    );

    EXPECT_FALSE(fs::exists(STAGING_DIR));
    EXPECT_TRUE(successReportReceived);
}
