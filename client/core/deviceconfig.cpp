#include "deviceconfig.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
}

DeviceConfig::DeviceConfig(fs::path confPath, fs::path lastUpdateFile)
    : confFile_(confPath)
    , lastUpdateFile_(lastUpdateFile)
{}


void DeviceConfig::loadConfig()
{
    std::ifstream configFile(confFile_, std::ios_base::binary);
    if (!configFile)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot open configfile " + confFile_.string());

    json devConfig = json::parse(configFile);

    type_                   = devConfig["type"]                        .get<std::string>();
    platform_               = devConfig["platform"]                    .get<std::string>();
    arch_                   = devConfig["arch"]                        .get<std::string>();
    pollingIntervalMinutes_ = devConfig["updatePollingIntervalMinutes"].get<int>();
    serverUrl_              = devConfig["serverURL"]                   .get<std::string>();
    serverPort_             = devConfig["serverPort"]                  .get<int>();
    certPath_               = devConfig["certPath"]                    .get<std::string>();
    keyPath_                = devConfig["keyPath"]                     .get<std::string>();
    caCertPath_             = devConfig["caCertPath"]                  .get<std::string>();
    publicKeyPath_          = devConfig["publicKeyPath"]               .get<std::string>();
    id_                     = devConfig.value("id", uint32_t());

    std::cout << "DeviceInfo: loaded config: \n"
                "\ttype = "            << type_                   << "\n"
                "\tplatform = "        << platform_               << "\n"
                "\tpollingInterval = " << pollingIntervalMinutes_ << "\n"
                "\tserverURL = "       << serverUrl_              << "\n"
                "\tserverPort = "      << serverPort_             << "\n"
                "\tcertPath = "        << certPath_               << "\n"
                "\tkeyPath = "         << keyPath_                << "\n"
                "\tcaCertPath = "      << caCertPath_             << "\n"
                "\tpublicKeyPath = "   << publicKeyPath_          << "\n"
                "\tid = "              << id_                     << std::endl;
}

void DeviceConfig::loadPrevManifest()
{
    fs::path recoveryPath = lastUpdateFile_.parent_path() / "rollback" /
                            lastUpdateFile_.filename();

    if (fs::exists(recoveryPath) == true)
    {
        std::ifstream lastManifestFile(recoveryPath,
                                       std::ios_base::in | std::ios_base::binary);

        if (!lastManifestFile)
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "Cannot open recovery last update file " + lastUpdateFile_.string());
        }

        std::cout << "Loading prev manifest from rollback path" << std::endl;
        lastManifestFile.open(recoveryPath,
                              std::ios_base::in | std::ios_base::binary);

        prevManifest_.loadFromJson(json::parse(lastManifestFile));
    }
    else
    {
        std::ifstream lastManifestFile(lastUpdateFile_,
                                       std::ios_base::in | std::ios_base::binary);
        if (!lastManifestFile)
        {
            if (errno == ENOENT)
            {

                std::cout << "UpdateContext: There is no prev manifest file. "
                             "Assumnig device has no software!"
                          << std::endl;
                return;
            }
            else
            {
                throw std::system_error(std::error_code(errno, std::generic_category()),
                                        "Cannot open last update file " + lastUpdateFile_.string());
            }
        }

        prevManifest_.loadFromJson(json::parse(lastManifestFile));
    }
}


void DeviceConfig::saveNewUpdateInfo(const ArtifactManifest& newManifest)
{
    std::ofstream lastManifestFile(lastUpdateFile_);
    if (!lastManifestFile)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot open last update file " + lastUpdateFile_.string());

    json data = newManifest.saveInJson();
    std::string stringRepresentation = data.dump();

    if (!lastManifestFile.write(stringRepresentation.data(), stringRepresentation.size()))
        throw std::runtime_error("Cannot write to file!");

    prevManifest_ = newManifest;
}


void DeviceConfig::saveId(uint32_t id)
{
    std::ifstream in(confFile_);
    if (!in)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot open configfile " + confFile_.string());

    json config = json::parse(in);
    in.close();

    config["id"] = id;

    std::ofstream out(confFile_);
    if (!out)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot open configfile " + confFile_.string());

    std::string stringRepresentation = config.dump(4);
    if (!out.write(stringRepresentation.data(), stringRepresentation.size()))
        throw std::runtime_error("Cannot write to file!");

    out.close();

    int dirFd = ::open(confFile_.parent_path().c_str(), O_DIRECTORY);
    if (dirFd >= 0)
    {
        ::fsync(dirFd);
        ::close(dirFd);
    }

    loadConfig();
}
