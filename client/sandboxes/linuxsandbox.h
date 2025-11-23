#ifndef LINUXSANDBOX_H
#define LINUXSANDBOX_H

#include "sandbox.h"
#include <string>
#include <sys/types.h>
#include <filesystem>

namespace fs = std::filesystem;

class LinuxSandbox : public Sandbox
{
public:
    LinuxSandbox(const fs::path& path) : path_(path) {}

    enum Responses : int { OK = 0, FAIL = -1 };
    enum Comands   : int { RUN = 1, RUN_NEXT = 2, ABORT = -1 };
    enum FDNames   : int { PARENT, CHILD, TOTAL };

    virtual void prepare(UpdateContext& context) override;
    virtual void launch(UpdateContext &context) override;
    virtual void cleanup(UpdateContext& context) override;

    virtual pid_t getPid() override { return pid_; };
    virtual fs::path getPath() override { return path_; };

private:
    void copyDependencies(const UpdateContext& context);
    void createRootfs(const UpdateContext& context);

    fs::path path_;
    pid_t pid_;

    int socketsFds_[TOTAL];
    void socketReport(int sockFd, int cmd, const std::string& logMessage);
    int  socketRead(int sockFd);
};





#endif // LINUXSANDBOX_H
