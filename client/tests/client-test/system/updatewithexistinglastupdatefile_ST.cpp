#include "systemfixturebase.h"


class UpdateWithExistingLastUpdateSystemTest : public SystemFixtureBase
{
protected:
    fs::path archivePath;
    std::string manifestString;
    std::vector<uint8_t> archiveData;
    std::atomic<bool> successReportReceived{false};
    fs::path prevManifestFile;

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();

        fs::create_directories(BASE_TEST_DIR / "opt/myapp");
        writeFile(BASE_TEST_DIR / "opt/myapp/app2", "APP2_BINARY_OLD");
        writeFile(BASE_TEST_DIR / "opt/myapp/app3", "APP3_BINARY_OLD");
        writeFile(BASE_TEST_DIR / "prepare.sh", "#!/bin/bash\necho prepare_old");
        writeFile(BASE_TEST_DIR / "commit.sh",  "#!/bin/bash\necho commit_old");

        createDeviceConfig(CONF_FILE, 102);

        prevManifestFile = BASE_TEST_DIR / "last_update.json";

        auto hashOf = [&](const fs::path& p)
        {
            return SSLUtilsAdapter().sha256FromFile(p.string());
        };

        json prevManifestJson =
        {
            { "release", {
                { "version",   "1.0.0" },
                { "type",      "Raspberry Pi 4" },
                { "timestamp", "2025-01-01T00:00:00Z" },
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

        writeFile(prevManifestFile, prevManifestJson.dump());

        createStateMachine(prevManifestFile, StateExecutor::REGISTRATION);

        fs::path newDir = BASE_TEST_DIR / "new_version";
        fs::create_directories(newDir);
        writeFile(newDir / "app2", "APP2_BINARY_NEW");
        writeFile(newDir / "app3", "APP3_BINARY_NEW");
        writeFile(newDir / "prepare.sh", "#!/bin/bash\necho prepare_new");
        writeFile(newDir / "commit.sh",  "#!/bin/bash\necho commit_new");

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
                    { "hash", {{ "algo", "sha256" }, { "value", ArtifactManifest::stringHashFromRaw(hashOf(newDir / "app2")) }}}
                },
                {
                    { "path", (BASE_TEST_DIR / "opt/myapp/app3").c_str() },
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

        manifestString = manifestJson.dump();

        fs::path manifestFile = BASE_TEST_DIR / "manifest.json";
        fs::path sigFile      = BASE_TEST_DIR / "manifest.json.sig";
        writeFile(manifestFile, manifestString);

        std::string signCmd =
            std::string("openssl dgst -sha256 -sign ") +
            PROJECT_ROOT_DIR +
            "/client/tests/resources/security/private.pem -out " +
            sigFile.string() + " " + manifestFile.string();

        if (system(signCmd.c_str()) != 0)
            throw std::runtime_error("Failed to sign manifest via OpenSSL");

        std::string signatureData = readFileBinary(sigFile);
        std::string signatureB64 =
            stateMachine->context.cryptoUtils->encodeBase64(
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
            (newDir / "app2").string(),
            (newDir / "app3").string(),
            (newDir / "prepare.sh").string(),
            (newDir / "commit.sh").string()
        };

        ASSERT_EQ(
            stateMachine->context.archiveTools->create_archive_from_paths(paths, archiveData),
            ARCHIVE_OK
        );

        server->Get("/manifest", [this, signatureString](const httplib::Request&, httplib::Response& res) {
            json response {
                { "manifest",  manifestString },
                { "signature", signatureString }
            };
            res.set_content(response.dump(), "application/json");
            res.status = httplib::StatusCode::OK_200;
        });

        server->Get("/download", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(
                reinterpret_cast<const char*>(archiveData.data()),
                archiveData.size(),
                "application/octet-stream"
            );
            res.status = httplib::StatusCode::OK_200;
        });

        server->Post("/report", [this](const httplib::Request& req, httplib::Response& res) {
            auto j = json::parse(req.body);
            if (j["status"] == "SUCCESS" && j["current_version"] == "2.0.0")
                successReportReceived = true;
            res.status = httplib::StatusCode::OK_200;
        });

        startServer();
    }
};

TEST_F(UpdateWithExistingLastUpdateSystemTest, ExecuteUpdateWhenPrevManifestExists)
{
    ASSERT_EQ(
        stateMachine->context.devconf->prevManifest().release.version,
        "1.0.0"
    );

    const int MAX_STEPS = StateExecutor::TOTAL + 2;
    int steps = 0;
    bool idleStateHasntPassed = true;

    while ((stateMachine->state() != StateExecutor::IDLE || idleStateHasntPassed) &&
           ++steps < MAX_STEPS)
    {
        if (stateMachine->state() == StateExecutor::IDLE)
            idleStateHasntPassed = false;

        stateMachine->run();
    }

    ASSERT_LT(steps, MAX_STEPS);

    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
    EXPECT_EQ(
        stateMachine->context.devconf->prevManifest().release.version,
        "2.0.0"
    );

    EXPECT_TRUE(successReportReceived);
    EXPECT_FALSE(fs::exists(STAGING_DIR));
    EXPECT_TRUE(fs::exists(BASE_TEST_DIR / "opt/myapp/app2"));
    EXPECT_TRUE(fs::exists(BASE_TEST_DIR / "opt/myapp/app3"));
    EXPECT_EQ(
        readFileBinary(BASE_TEST_DIR / "opt/myapp/app2"),
        "APP2_BINARY_NEW"
    );
    EXPECT_EQ(
        readFileBinary(BASE_TEST_DIR / "opt/myapp/app3"),
        "APP3_BINARY_NEW"
    );
}
