#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include "artifactmanifest.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::ordered_json;

/// Класс, хранящий информацию об устройстве
class DeviceInfo
{
public:
    DeviceInfo();

    void saveNewUpdateInfo(const ArtifactManifest& newManifest);

    auto type() const { return type_; }
    auto place() const { return place_; }
    auto polingIntervalMinutes() const { return pollingIntervalMinutes_; }
    auto serverUrl() const { return serverUrl_; }
    auto prevManifest() const { return prevManifest_; }

private:
    std::string      type_;
    std::string      place_;
    int              pollingIntervalMinutes_;
    std::string      serverUrl_;
    ArtifactManifest prevManifest_;
};

#endif // DEVICEINFO_H
