#include "uploadservice.h"
#include "../../utils/common/archive.h"
#include "database.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <filesystem>


using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

namespace {
    const fs::path BUFFER_DIR = "/tmp/pco-buffer";
    const fs::path STORAGE_DIR = "/opt/pco/storage";

    auto toPgArray = [](const std::vector<std::string> &v) {
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


void UploadService::parseManifest(const std::string& raw)
{
    json        data         = json::parse(raw);
    std::string manifestRaw  = data["manifest"];
    std::string signatureRaw = data["signature"];

    json manifest = json::parse(manifestRaw);

    entry_.manifest  = manifestRaw;
    entry_.signature = signatureRaw;
    entry_.version   = manifest["release"]["version"];
    entry_.type      = manifest["release"]["type"];
    entry_.platform  = manifest["release"]["platform"];
    entry_.arch      = manifest["release"]["arch"];

    manifestParsed = true;
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
            std::cout << "FILE: " << entry.path().string() << "\n";
            fs::path serverPath = STORAGE_DIR / entry_.type / entry_.arch / entry_.platform / entry_.version;

            if (fs::create_directories(serverPath) == false && fs::exists(serverPath) == false)
                throw std::runtime_error("Cannot create storage dir");

            fs::copy(entry.path(), serverPath);

            entry_.paths.push_back(serverPath.string());
        }
    }

    filesParsed = entry_.paths.empty() == false;

    fs::remove_all(BUFFER_DIR);
}

void UploadService::commit()
{
    if (filesParsed == false || manifestParsed == false)
    {
        throw std::runtime_error("Cannot update");
    }

    pqxx::work txn(*Database::instance().connection());
    std::string paths = toPgArray(entry_.paths);
    pqxx::params params;
    params.append(entry_.manifest);
    params.append(entry_.signature);
    params.append(entry_.version);
    params.append(entry_.type);
    params.append(entry_.platform);
    params.append(entry_.arch);
    params.append(paths);

    pqxx::result res = txn.exec(
        "INSERT INTO releases (manifest_raw, signature_raw, version, device_type, platform, arch, file_paths) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7) "
        "RETURNING id;", params);

    txn.commit();
}
