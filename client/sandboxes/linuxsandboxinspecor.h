#ifndef LINUXSANDBOXINSPECOR_H
#define LINUXSANDBOXINSPECOR_H

#include <filesystem>
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
        bool oom;

        static uint64_t readMemory(const fs::path& path);
        static uint64_t readCpu(const fs::path& path);
        static uint64_t readThrottle(const fs::path& path);
        static bool readOom(const fs::path& path);
        static uint64_t readInt(const fs::path& path);
    };

public:
    LinuxSandboxInspector(): groupPath_("/sys/fs/cgroup/pco") {}

    void createCgroup(UpdateContext& context);
    virtual void inspect(UpdateContext& context) override;

    Sample sample();

    bool hasFinished(pid_t pid, std::string filename);
    fs::path basepath() { return groupPath_; }

    void writeToFile(const fs::path& path, const std::string& value);
private:
    fs::path createCgroupsHierarchy();
    fs::path groupPath_;
};



#endif // LINUXSANDBOXINSPECOR_H
