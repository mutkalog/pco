#ifndef LINUXSANDBOX_H
#define LINUXSANDBOX_H

#include "sandbox.h"
#include <string>
#include <sys/types.h>

class LinuxSandbox : public Sandbox
{
public:
    LinuxSandbox() = default;

    enum Responses : int { OK = 0, FAIL = -1 };
    enum Comands   : int { RUN = 0, ABORT = -1 };
    enum FDNames   : int { PARENT, CHILD, TOTAL };

    // void run(const UpdateContext& context) {prepare(context); launch(context); }

    virtual void prepare(UpdateContext& context) override;
    virtual void launch(UpdateContext &context) override;
    virtual void cleanup(UpdateContext& context) override;

private:
    void copyDependencies(const UpdateContext& context);
    void createRootfs(const UpdateContext& context);
    pid_t containerPid_;
    int socketsFds_[TOTAL];

    void socketReport(int sockFd, int cmd, const std::string& logMessage);
    int  socketRead(int sockFd);

};





#endif // LINUXSANDBOX_H
