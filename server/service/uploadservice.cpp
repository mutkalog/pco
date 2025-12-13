#include "uploadservice.h"
#include "../../utils/common/archivetools.h"
#include "database.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>


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

void UploadService::upload(std::shared_ptr<ServerContext>& sc, std::optional<int> canaryPercentage,
                           int requiredTimeMinutes,
                           const std::string &manifest,
                           const std::string &archive)
{

    std::stringstream ss;
    ss << std::this_thread::get_id();
    fs::path updateBufDir = BUFFER_DIR / ss.str();
    fs::path storagePath;

    try
    {
        ReleasesTableEntry entry;

        parseManifest(manifest, entry);
        parseFiles(archive, entry, updateBufDir, storagePath);
        commit(sc, canaryPercentage, requiredTimeMinutes, entry, updateBufDir);
    }
    catch (const pqxx::sql_error& ex)
    {
        if (fs::exists(updateBufDir) == true)
        {
            fs::remove_all(updateBufDir);
        }

        throw;
    }
    catch (...)
    {
        if (fs::exists(updateBufDir) == true)
        {
            fs::remove_all(updateBufDir);
        }
        if (storagePath.empty() == false && fs::exists(storagePath) == true)
        {
            fs::remove_all(storagePath);
        }

        throw;
    }
}

void UploadService::parseManifest(const std::string& raw, ReleasesTableEntry& entry)
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

    entry.manifest  = manifestRaw;
    entry.signature = signatureRaw;
    entry.version   = manifest["release"]["version"];
    entry.type      = manifest["release"]["type"];
    entry.platform  = manifest["release"]["platform"];
    entry.arch      = manifest["release"]["arch"];
    std::cout << "version " << entry.version << std::endl;
}

void UploadService::parseFiles(const std::string &raw, ReleasesTableEntry &entry, const fs::path& bufDir, fs::path& storagePath)
{
    std::vector<uint8_t> data(raw.size());
    std::memcpy(data.data(), raw.data(), raw.size());

    fs::remove_all(bufDir);

    if (fs::create_directories(bufDir) == false && fs::exists(bufDir) == false)
        throw std::runtime_error("Cannot create temporary dir");

    if (fs::create_directories(STORAGE_DIR) == false && fs::exists(STORAGE_DIR) == false)
        throw std::runtime_error("Cannot create storage dir");

    if (extract(data.data(), data.size(), bufDir.c_str()) != 0)
    {
        throw std::runtime_error("Cannot extract files from archive");
    }

    for (const auto& file : fs::directory_iterator(bufDir))
    {
        if (file.is_regular_file())
        {
            fs::path serverPath = STORAGE_DIR / entry.type / entry.arch / entry.platform / entry.version;
            storagePath = serverPath;

            if (fs::create_directories(serverPath) == false && fs::exists(serverPath) == false)
                throw std::runtime_error("Cannot create storage dir");

            entry.bufferPathToStoragePath.push_back({file.path(), serverPath});
            entry.storagePaths.push_back(serverPath / file.path().filename());
        }
    }
}

void UploadService::commit(std::shared_ptr<ServerContext>& sc,
                           std::optional<int> canaryPercentage,
                           int requiredTimeMinutes,
                           ReleasesTableEntry &entry,
                           const fs::path& bufDir)
{
    auto conn = cp_.acquire();
    try
    {
        if (entry.storagePaths.empty() == true)
        {
            throw std::runtime_error("Cannot update");
        }

        pqxx::work txn(*conn);

        std::string paths = toPgArray(entry.storagePaths);
        pqxx::params params;
        params.append(entry.manifest);
        params.append(entry.signature);
        params.append(entry.version);
        params.append(entry.type);
        params.append(entry.platform);
        params.append(entry.arch);
        params.append(paths);
        params.append(canaryPercentage.has_value());
        params.append(canaryPercentage.has_value() == true ? *canaryPercentage : 0);
        params.append(requiredTimeMinutes);

        pqxx::result res = txn.exec(
            "INSERT INTO releases (manifest_raw, signature_raw, version, device_type, platform, arch, file_paths, is_canary, canary_percent, installation_time) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) "
            "RETURNING id;", params);

        if (res.empty() == true)
            throw std::runtime_error("Empty result on INSERT INTO releases");

        auto newId = res[0]["id"].as<uint64_t>();


        const auto& map = entry.bufferPathToStoragePath;
        for (size_t i = 0; i != map.size(); ++i)
        {
            fs::copy(map[i].first, map[i].second);

            ///@todo убрать
            std::cout << entry.storagePaths[i] << std::endl;
        }
        std::cout << "Files successfuly transfered" << std::endl;

        txn.commit();

        fs::remove_all(bufDir);

        auto& rollouts = sc->staging.rollouts;

        std::lock_guard<std::mutex> lg(rollouts.mtx);
        static_cast<void>(lg);

        RolloutInfo ri {
            entry.type,
            entry.platform,
            entry.arch,
            canaryPercentage.has_value() == true,
            canaryPercentage.has_value() == true ? *canaryPercentage : 0,
            canaryPercentage.has_value() == true ? *canaryPercentage : 0,
        };

        rollouts.releaseToInfoMap.insert({newId, std::move(ri)});
        rollouts.cv.notify_all();

        cp_.release(std::move(conn));
    }
    catch (...)
    {
        cp_.release(std::move(conn));
        throw;
    }
}

