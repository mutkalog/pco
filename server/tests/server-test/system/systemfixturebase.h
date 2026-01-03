#ifndef SYSTEMFIXTUREBASE_H
#define SYSTEMFIXTUREBASE_H

#include "core/database.h"
#include "core/deviceconfig.h"
#include <httplib.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <fstream>

namespace fs = std::filesystem;

class SystemFixtureBase : public testing::Test
{
protected:
    const fs::path BASE_TEST_DIR = fs::temp_directory_path() / "pco";

    const fs::path CLIENT_CERT = fs::path(PROJECT_ROOT_DIR) / "server/sim/security/client.crt";
    const fs::path CLIENT_KEY  = fs::path(PROJECT_ROOT_DIR) / "server/sim/security/client.key";
    const fs::path CA_CERT     = fs::path(PROJECT_ROOT_DIR) / "server/sim/security/ca.pem";

    const std::string UPLOAD_PATH   = "/upload";
    const std::string REGISTER_PATH = "/register";
    const std::string MANIFEST_PATH = "/manifest";
    const std::string DOWNLOAD_PATH = "/download";
    const std::string REPORT_PATH   = "/report";

    std::string dockerImageName = "pco-server-app";
    std::string dockerContainerId;

    std::unique_ptr<httplib::SSLClient> client;
    const int serverPort = 39024;

    void SetUp() override
    {
        try { clearDatabase(); }
        catch (...) { }

        fs::create_directories(BASE_TEST_DIR);

        client = std::make_unique<httplib::SSLClient>("localhost", serverPort,
                                                      CLIENT_CERT, CLIENT_KEY);
        client->set_ca_cert_path(CA_CERT);
        client->enable_server_certificate_verification(true);

        if (!client->is_valid())
        {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("Failed to initialize SSL client");
        }
    }

    void TearDown() override
    {
        try { clearDatabase(); }
        catch (...) { }

        fs::remove_all(BASE_TEST_DIR);
        stopDockerContainer();
    }

    httplib::Result postMultipartUpload(const std::string& path,
                                    const std::string& manifestRaw,
                                    const std::string& signatureJsonString,
                                    const std::vector<uint8_t>& archiveBinary,
                                    const httplib::Params& params = {})
    {
        json wrapper;
        wrapper["manifest"] = manifestRaw;
        wrapper["signature"] = signatureJsonString;

        std::string wrapperStr = wrapper.dump();

        httplib::UploadFormDataItems items;
        items.push_back({ "manifest", wrapperStr, "manifest.json", "application/json" });

        std::string archiveStr(reinterpret_cast<const char*>(archiveBinary.data()), archiveBinary.size());
        items.push_back({ "archive", archiveStr, "update.tar.gz", "application/gzip" });

        std::string full_path = path;
        if (!params.empty())
        {
            full_path += "?";
            bool first = true;
            for (const auto& [k, v] : params)
            {
                if (!first) full_path += "&";
                full_path += k + "=" + v;
                first = false;
            }
        }

        return client->Post(full_path.c_str(), items);
    }

    void waitForServerUp(int timeout_ms)
    {
        int waited = 0;
        while (waited < timeout_ms)
        {
            try {
                auto res = client->Get("/");
                if (res && res->status >= 200)
                    return;
            } catch (...) {}
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            waited += 200;
        }
        throw std::runtime_error("Server did not become available in time");
    }

    bool sendDeviceReport(int deviceId,
                          const std::string& type,
                          const std::string& arch,
                          const std::string& platform,
                          const std::string& status,
                          int ec,
                          const std::string& version)
    {
        json j;
        j["id"]       = deviceId;
        j["type"]     = type;
        j["arch"]     = arch;
        j["platform"] = platform;
        j["status"]   = status;
        j["error"]    = {
            { "code",    ec },
            { "message", "" }
        };
        j["current_version"] = version;

        auto res = client->Post(REPORT_PATH.c_str(), j.dump(), "application/json");

        return res && res->status == 200;
    }

    std::string signAndBase64(const fs::path& manifestFile, const fs::path& tmpDir)
    {
        fs::path sigFile = tmpDir / "manifest.sig";
        fs::path sigB64  = tmpDir / "manifest.sig.b64";

        std::string privKey = std::string(PROJECT_ROOT_DIR) + "/server/sim/security/private.pem";
        std::string cmdSign = "openssl dgst -sha256 -sign " + privKey + " -out " + sigFile.string() + " " + manifestFile.string();
        if (system(cmdSign.c_str()) != 0)
        {
            throw std::runtime_error("openssl sign failed");
        }

        std::string cmdB64 = "openssl base64 -in " + sigFile.string() + " -A -out " + sigB64.string();
        if (system(cmdB64.c_str()) != 0)
        {
            throw std::runtime_error("openssl base64 failed");
        }

        return readFileBinary(sigB64);
    }

    void clearDatabase()
    {
        pqxx::connection conn("dbname=pco_test user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        txn.exec("TRUNCATE releases, devices, release_assignments, reports RESTART IDENTITY CASCADE;");
        txn.commit();
    }

    void writeFile(const fs::path& path, const std::string& data)
    {
        fs::create_directories(path.parent_path());
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs)
            throw std::runtime_error("cannot open file for writing: " + path.string());
        ofs.write(data.data(), data.size());
    }

    std::string readFileBinary(const fs::path& path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs)
            throw std::runtime_error("cannot open file for reading: " + path.string());
        std::ostringstream ss;
        ss << ifs.rdbuf();
        return ss.str();
    }

    void buildDockerImage(const fs::path& pathToDockerfile)
    {
        std::string cmd = "docker build -t " + dockerImageName + " " + pathToDockerfile.string();
        int ret = system(cmd.c_str());
        if (ret != 0)
            throw std::runtime_error("Failed to build docker image: " + dockerImageName);
    }

    void runDockerContainer()
    {
        std::string cmd = "docker run -d --network host " + dockerImageName;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe)
            throw std::runtime_error("Failed to start docker container");

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            dockerContainerId = std::string(buffer);
        pclose(pipe);

        dockerContainerId.erase(dockerContainerId.find_last_not_of("\n") + 1);
    }

    void stopDockerContainer()
    {
        if (dockerContainerId.empty())
            return;

        std::string cmd = "docker rm -f " + dockerContainerId;
        system(cmd.c_str());
        dockerContainerId.clear();
    }
};

#endif // SYSTEMFIXTUREBASE_H
