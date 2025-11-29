#include "deviceinfo.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>


namespace fs = std::filesystem;

namespace {
    const fs::path CONF_PATH = "/var/pco/devconfig.json";
    const fs::path LAST_UPDATE_INFO_PATH = "/var/pco/last-update.json";
}

DeviceInfo::DeviceInfo()
{
    std::ifstream configfile(CONF_PATH, std::ios_base::binary);
    if (!configfile)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot open configfile " + CONF_PATH.string());

    try
    {
        json devConfig = json::parse(configfile);

        type_                   = devConfig["type"].get<std::string>();
        place_                  = devConfig["place"].get<std::string>();
        pollingIntervalMinutes_ = devConfig["updatePollingIntervalMinutes"].get<int>();
        serverUrl_              = devConfig["serverURL"].get<std::string>();
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        exit(77);
        throw;
    }
    std::cout << "DeviceInfo: loaded config: \n"
                "\ttype = "            << type_                   << "\n"
                "\tplace = "           << place_                  << "\n"
                "\tpollingInterval = " << pollingIntervalMinutes_ << "\n"
                "\tserverURL = "       << serverUrl_              << std::endl;

    std::ifstream lastManifestFile(LAST_UPDATE_INFO_PATH, std::ios_base::binary);
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
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "Cannot open last update file " + LAST_UPDATE_INFO_PATH.string());
    }

    prevManifest_.loadFromJson(json::parse(lastManifestFile));
}

void DeviceInfo::saveNewUpdateInfo(const ArtifactManifest& newManifest)
{
    std::ofstream lastManifestFile(LAST_UPDATE_INFO_PATH);
    if (!lastManifestFile)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot open last update file " + LAST_UPDATE_INFO_PATH.string());

    json data = newManifest.saveInJson();
    std::string stringRepresentation = data.dump();

    if (!lastManifestFile.write(stringRepresentation.data(), stringRepresentation.size()))
        throw std::runtime_error("DeviceInfo: Cannot write to file!");
}





















