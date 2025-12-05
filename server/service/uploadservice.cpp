#include "uploadservice.h"
#include "../../utils/common/archive.h"
#include "database.h"
#include <iostream>
#include <nlohmann/json.hpp>


using json = nlohmann::ordered_json;

namespace {
    const fs::path BUFFER_DIR  = "/tmp/pco-buffer";
    const fs::path STORAGE_DIR = "/opt/pco/storage";

    auto toPgArray = [](const std::vector<fs::path> &v) {
        std::string res = "{";
        for (size_t i = 0; i < v.size(); ++i) {

        std::string s = v[i];
        size_t pos = 0;
        while ((pos = s.find('"', pos)) != std::string::npos) {
            s.insert(pos, "\\");
            pos += 2;
        }
        pos = 0;
        while ((pos = s.find('\\', pos)) != std::string::npos) {
            s.insert(pos, "\\");
            pos += 2;
        }

        res += '"' + s + '"';
        if (i + 1 != v.size())
            res += ",";
        }
        res += "}";
        return res;
    };

} // namespace

void UploadService::upload(ServerContext* sc, std::optional<int> canaryPercentage,
                           const std::string &manifest,
                           const std::string &archive)
{
        parseManifest(manifest);
        parseFiles(archive);
        commit(sc, canaryPercentage);
}

void UploadService::parseManifest(const std::string& raw)
{
    if (raw.empty())
        throw std::runtime_error("Empty manifest");

    json data = json::parse(raw);

    if (data.contains("manifest") == false || data["manifest"].is_string() == false)
        throw std::runtime_error("Manifest field is missing or not a string");

    if (data.contains("signature") == false || data["signature"].is_string() == false)
        throw std::runtime_error("Signature field is missing or not a string");

    std::string manifestRaw  = data["manifest"];
    std::string signatureRaw = data["signature"];

    json manifest = json::parse(manifestRaw);

    entry_.manifest  = manifestRaw;
    entry_.signature = signatureRaw;
    entry_.version   = manifest["release"]["version"];
    entry_.type      = manifest["release"]["type"];
    entry_.platform  = manifest["release"]["platform"];
    entry_.arch      = manifest["release"]["arch"];
}

void UploadService::parseFiles(const std::string &raw)
{
    std::vector<uint8_t> data(raw.size());
    std::memcpy(data.data(), raw.data(), raw.size());

    fs::remove_all(BUFFER_DIR);

    if (fs::create_directories(BUFFER_DIR) == false && fs::exists(BUFFER_DIR) == false)
        throw std::runtime_error("Cannot create temporary dir");

    if (fs::create_directories(STORAGE_DIR) == false && fs::exists(STORAGE_DIR) == false)
        throw std::runtime_error("Cannot create storage dir");

    if (extract(data.data(), data.size(), BUFFER_DIR.c_str()) != 0)
    {
        throw std::runtime_error("Cannot extract files from archive");
    }

    for (const auto& entry : fs::directory_iterator(BUFFER_DIR))
    {
        if (entry.is_regular_file())
        {
            fs::path serverPath = STORAGE_DIR / entry_.type / entry_.arch / entry_.platform / entry_.version;

            if (fs::create_directories(serverPath) == false && fs::exists(serverPath) == false)
                throw std::runtime_error("Cannot create storage dir");

            entry_.bufferPathToStoragePath.push_back({entry.path(), serverPath});
            entry_.storagePaths.push_back(serverPath / entry.path().filename());
        }
    }
}

void UploadService::commit(ServerContext* sc, std::optional<int> canaryPercentage)
{
    if (entry_.storagePaths.empty() == true)
    {
        throw std::runtime_error("Cannot update");
    }

    pqxx::work txn(*conn_);

    std::string paths = toPgArray(entry_.storagePaths);
    pqxx::params params;
    params.append(entry_.manifest);
    params.append(entry_.signature);
    params.append(entry_.version);
    params.append(entry_.type);
    params.append(entry_.platform);
    params.append(entry_.arch);
    params.append(paths);
    params.append(canaryPercentage.has_value());
    params.append(canaryPercentage.has_value() == true ? *canaryPercentage : 0);

    pqxx::result res = txn.exec(
        "INSERT INTO releases (manifest_raw, signature_raw, version, device_type, platform, arch, file_paths, is_canary, canary_percent) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) "
        "RETURNING id;", params);

    if (res.empty() == true)
        throw std::runtime_error("Empty result on INSERT INTO releases");

    auto newId = res[0]["id"].as<uint64_t>();

    // if (canaryPercentage.has_value() == true)
    // {
        auto& rollouts = sc->data->staging.rollouts;

        std::lock_guard<std::mutex> lg(rollouts.mtx);
        static_cast<void>(lg);

        RolloutInfo ri{
            entry_.type,
            entry_.platform,
            entry_.arch,
            canaryPercentage.has_value() == true,
            canaryPercentage.has_value() == true ? *canaryPercentage : 0,
            canaryPercentage.has_value() == true ? *canaryPercentage : 0,
        };

        rollouts.releaseToInfoMap.insert({newId, std::move(ri)});
        rollouts.cv.notify_one();
    // }
    // else
    // {
    //     pqxx::params updateParams;
    //     updateParams.append(newId);
    //     updateParams.append(entry_.type);
    //     updateParams.append(entry_.platform);
    //     updateParams.append(entry_.arch);

    //     txn.exec(
    //         "UPDATE releases "
    //         "SET active = false "
    //         "WHERE device_type = $2 "
    //         "  AND platform = $3 "
    //         "  AND arch = $4 "
    //         "  AND id <> $1;", updateParams);

    // }


    const auto& map = entry_.bufferPathToStoragePath;
    for (size_t i = 0; i != map.size(); ++i)
    {
        fs::copy(map[i].first, map[i].second);
        if (fs::exists(entry_.storagePaths[i]) == false)
            throw std::runtime_error("File was not copied");

        ///@todo убрать
        std::cout << entry_.storagePaths[i] << std::endl;
    }
    std::cout << "Files successfuly transfered" << std::endl;

    fs::remove_all(BUFFER_DIR);
    txn.commit();

    cleanupEntry();
}

