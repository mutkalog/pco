#ifndef SYSTEMFIXTUREBASE_H
#define SYSTEMFIXTUREBASE_H

#include <httplib.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <fstream>


namespace fs = std::filesystem;

class SystemFixtureBase : public testing::Test
{
protected:
    const fs::path BASE_TEST_DIR = fs::temp_directory_path() / "pco";

    const fs::path CLIENT_CERT = fs::path(PROJECT_ROOT_DIR) / "client/sim/security/server.crt";
    const fs::path CLIENT_KEY  = fs::path(PROJECT_ROOT_DIR) / "client/sim/security/server.key";
    const fs::path CA_CERT     = fs::path(PROJECT_ROOT_DIR) / "client/sim/security/ca.crt";

    std::string dockerImageName = "pco-client-app";
    std::string dockerContainerId;

    std::unique_ptr<httplib::SSLServer> server;
    std::thread serverThread;
    const int serverPort = 18992;

    void SetUp() override
    {
        fs::create_directories(BASE_TEST_DIR);
        initServer();
    }

    void TearDown() override
    {
        stopServer();
        fs::remove_all(BASE_TEST_DIR);
        stopDockerContainer();
    }

    void initServer()
    {
        server = std::make_unique<httplib::SSLServer>(
            CLIENT_CERT.string().c_str()
            , CLIENT_KEY.string().c_str()
        );

        if (!server->is_valid())
        {
            throw std::runtime_error("Failed to initialize SSL server");
        }
    }

    void startServer()
    {
        if (serverThread.joinable()) return;

        serverThread = std::thread([this]() {
            server->listen("0.0.0.0", serverPort);
            std::cout << "server done " << std::endl;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    void stopServer()
    {
        if (server)
        {
            server->stop();
        }
        if (serverThread.joinable())
        {
            serverThread.join();
        }
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
