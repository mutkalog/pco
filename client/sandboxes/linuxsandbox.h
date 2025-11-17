#ifndef LINUXSANDBOX_H
#define LINUXSANDBOX_H

#include "sandbox.h"

class LinuxSandbox : public Sandbox
{
public:
    LinuxSandbox() = default;

    enum Responses : int { OK = 0, FAIL = -1 };
    enum Comands   : int { RUN = 0, ABORT = -1 };
    enum FDNames   : int { PARENT, CHILD, TOTAL };

    void run(const UpdateContext& context) {prepare(context); launch(context); }
protected:
    virtual void prepare(const UpdateContext& context) override;
    virtual void launch(const UpdateContext& context) override;
    virtual void cleanup(const UpdateContext& context) override { }

private:
    void copyDependencies(const UpdateContext& context);
    void createRootfs(const UpdateContext& context);
    pid_t containerPid_;
    int socketsFds_[TOTAL];

};

#endif // LINUXSANDBOX_H
