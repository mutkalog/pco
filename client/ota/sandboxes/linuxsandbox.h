#ifndef LINUXSANDBOX_H
#define LINUXSANDBOX_H

#include "sandbox.h"
#include <optional>
#include <string>
#include <sys/types.h>
#include <filesystem>

namespace fs = std::filesystem;

enum MessageType : int { STATUS_CODE, COMMAND, PROCESS_EXIT_STATUS, PROCESS_ID };

struct Message {
    int type;
    int status;
    int command;
    struct
    {
        int pid;
        int exitCode;
    } processInfo;
};

using raw_message_t = std::array<char, sizeof(Message)>;


class LinuxSandbox : public Sandbox
{
public:
    LinuxSandbox(const fs::path& path) : path_(path), busyResources_() {}

    enum Responses : int { OK = 0, FAIL = -1 };
    enum Comands   : int { RUN = 1, RUN_NEXT = 2, ABORT = -1 };
    enum FDNames   : int { PARENT, CHILD, TOTAL };

    virtual void prepare(UpdateContext& context) override;
    virtual void launch(UpdateContext &context) override;
    virtual void cleanup(UpdateContext& context) override;

    virtual pid_t getPid() override { return pid_; };
    virtual fs::path getPath() override { return path_; };
    
    virtual std::pair<pid_t, int> getReturnCode() override;

private:
    void buildSandbox();
    void copyDependencies(const UpdateContext& context);

    std::string getInterpretator(const fs::path& path);
    std::optional<std::string> parseInterpretatorLine(const std::string &line);

    void createRootfs(const UpdateContext& context);

    fs::path path_;
    pid_t pid_;

    int socketsFds_[TOTAL];
    void socketReport(int sockFd, Message msg, const std::string &logMessage);
    Message socketRead(int sockFd);

    struct
    {
        uint16_t sandbox  : 1;
        uint16_t reserved : 15;
    } busyResources_;
};





#endif // LINUXSANDBOX_H
