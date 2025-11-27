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

    const auto type() { return type_ ;}
    const auto place() { return place_ ;}
    const auto polingIntervalMinutes() { return pollingIntervalMinutes_ ;}
    const auto serverUrl() { return serverUrl_ ;}
    const auto prevManifest() { return prevManifest_ ;}

private:
    std::string      type_;
    std::string      place_;
    int              pollingIntervalMinutes_;
    std::string      serverUrl_;
    ArtifactManifest prevManifest_;
};

#endif // DEVICEINFO_H
