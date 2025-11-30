#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include "artifactmanifest.h"
#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>

using json = nlohmann::ordered_json;

namespace fs = std::filesystem;

/// Класс, хранящий информацию об устройстве
class DeviceInfo
{
public:
    DeviceInfo();

    void saveNewUpdateInfo(const ArtifactManifest& newManifest);
    void saveId(uint32_t id);

    auto type() const { return type_; }
    auto platform() const { return platform_; }
    auto arch() const { return arch_; }
    auto polingIntervalMinutes() const { return pollingIntervalMinutes_; }
    auto serverUrl() const { return serverUrl_; }
    auto serverPort() const { return serverPort_; }
    auto certPath() const { return certPath_; }
    auto keyPath() const { return keyPath_; }
    auto caCertPath() const { return caCertPath_; }
    auto id() const { return id_; }
    auto prevManifest() const { return prevManifest_; }

private:
    std::string      type_;
    std::string      platform_;
    std::string      arch_;
    int              pollingIntervalMinutes_;
    std::string      serverUrl_;
    int              serverPort_;
    fs::path         certPath_;
    fs::path         keyPath_;
    fs::path         caCertPath_;
    uint32_t         id_;
    ArtifactManifest prevManifest_;

    void loadConfig();
};

#endif // DEVICEINFO_H
