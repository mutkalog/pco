#include "integrationfixturebase.h"


class CheckingExecutorIntegrationTest : public IntegrationFixtureBase
{
protected:
    fs::path manifestFile;
    fs::path sigFile;
    fs::path prevManifestFile;

    void SetUp() override
    {
        IntegrationFixtureBase::SetUp();

        manifestFile     = BASE_TEST_DIR / "manifest.json";
        sigFile          = BASE_TEST_DIR / "manifest.sig";
        prevManifestFile = BASE_TEST_DIR / "last-update.json";

        std::string manifestContent =
            R"({"release": { "version": "1.2.0", "type": "Raspberry Pi 4", "timestamp": "2025-11-12T10:23:00Z", "platform": "Linux", "arch": "ARMv8" }, "files": []})";

        writeFile(manifestFile, manifestContent);

        system((std::string("openssl dgst -sha256 -sign ") + PROJECT_ROOT_DIR +
                "/keys/private.pem -out " + sigFile.string() + " " + manifestFile.string()).c_str());

        createDeviceConfig(CONF_FILE, 222);
        createStateMachine(prevManifestFile, StateExecutor::CHECKING);

    }
};

TEST_F(CheckingExecutorIntegrationTest, ExecuteTransitToDownloading)
{
    server->Get("/manifest",
        [this](const httplib::Request&, httplib::Response& res)
        {
            std::string manifestData  = readFileBinary(manifestFile);
            std::string signatureData = readFileBinary(sigFile);
            std::string signatureB64  = SSLUtils::encodeBase64(
                std::vector<uint8_t>(signatureData.begin(), signatureData.end())
            );

            json signatureJson = {
                {"signature", {
                    {"algo", "rsa-sha256"},
                    {"keyname", "main-signing-key"},
                    {"value", signatureB64}
                }}
            };

            json response = {
                {"manifest", manifestData},
                {"signature", signatureJson.dump()}
            };

            res.set_content(response.dump(), "application/json");
            res.status = httplib::OK_200;
        });

    startServer();

    stateMachine->run();
    EXPECT_EQ(stateMachine->state(), StateExecutor::DOWNLOADING);
}

TEST_F(CheckingExecutorIntegrationTest, ExecuteAlreadyInstalledTransitToIdle)
{
    server->Get("/manifest",
        [this](const httplib::Request&, httplib::Response& res)
        {
            std::string manifestData  = readFileBinary(manifestFile);
            std::string signatureData = readFileBinary(sigFile);
            std::string signatureB64  = SSLUtils::encodeBase64(
                std::vector<uint8_t>(signatureData.begin(), signatureData.end())
            );

            json signatureJson = {
                {"signature", {
                    {"algo", "rsa-sha256"},
                    {"keyname", "main-signing-key"},
                    {"value", signatureB64}
                }}
            };

            json response = {
                {"manifest", manifestData},
                {"signature", signatureJson.dump()}
            };

            res.set_content(response.dump(), "application/json");
            res.status = httplib::OK_200;
        });

    startServer();

    auto& devconf = stateMachine->context.devconf;
    ArtifactManifest manifest;
    manifest.loadFromJson(json::parse(readFileBinary(manifestFile)));
    devconf->saveNewUpdateInfo(manifest);

    stateMachine->run();
    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
}

TEST_F(CheckingExecutorIntegrationTest, ExecuteInvalidSignatureTransitToIdle)
{
    server->Get("/manifest",
        [this](const httplib::Request&, httplib::Response& res)
        {
            std::string manifestData = readFileBinary(manifestFile);
            json signatureJson = {
                {"signature", {
                    {"algo", "rsa-sha256"},
                    {"keyname", "main-signing-key"},
                    {"value", "ZmFrZS1zaWduYXR1cmU="}
                }}
            };

            json response = {
                {"manifest", manifestData},
                {"signature", signatureJson.dump()}
            };

            res.set_content(response.dump(), "application/json");
            res.status = httplib::OK_200;
        });

    startServer();

    createStateMachine(prevManifestFile, StateExecutor::CHECKING);
    stateMachine->run();
    EXPECT_EQ(stateMachine->state(), StateExecutor::IDLE);
}
