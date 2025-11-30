#ifndef LINUXSANDBOXINSPECOR_H
#define LINUXSANDBOXINSPECOR_H

#include <filesystem>
#include <fstream>
#include "sandbox.h"

namespace fs = std::filesystem;

class LinuxSandboxInspector : public SandboxInspector
{
public:
    struct Sample
    {
        uint64_t memoryCurrent;
        uint64_t cpuUsage;
        uint64_t throttledUsec;
        uint64_t oom;

        template <typename T>
        static T readKeyValue(const fs::path& path, const std::string& targetKey);

        template <typename T>
        static T readValue(const fs::path& path);
    };


public:
    LinuxSandboxInspector(): groupPath_("/sys/fs/cgroup/pco"), busyResources_() {}

    void createCgroup(UpdateContext& context);
    virtual void inspect(UpdateContext& context) override;
    virtual void cleanup(UpdateContext& context) override;

    Sample sample();

    bool hasFinished(pid_t pid, std::string filename);
    bool hasFinished();
    fs::path basepath() { return groupPath_; }
    fs::path groupdir() { return groupPath_ / "cont"; }
    
    void writeToFile(const fs::path& path, const std::string& value, bool append = false);

private:
    fs::path createCgroupsHierarchy();
    fs::path groupPath_;
    struct
    {
        uint16_t groupPathCreated : 1;
        uint16_t cgroupMounted    : 1;
        uint16_t groupDirCreated  : 1;
        uint16_t processesAdded   : 1;
        uint16_t reserved         : 12;
    } busyResources_;
};

template<typename T>
inline T LinuxSandboxInspector::Sample::readValue(const fs::path &path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open: " + path.string());

    T value{};
    file >> value;

    return value;
}

template<typename T>
inline T LinuxSandboxInspector::Sample::readKeyValue(const fs::path &path, const std::string &targetKey)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open: " + path.string() + "/memory.envents");

    T value;
    std::string key;

    while (file >> key >> value)
    {
        if (key == targetKey)
            return value;
    }

    throw std::runtime_error("Cannot find key " + targetKey + " in " + path.string());
}


#endif // LINUXSANDBOXINSPECOR_H
