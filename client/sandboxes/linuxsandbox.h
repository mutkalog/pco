#ifndef LINUXSANDBOX_H
#define LINUXSANDBOX_H

#include "sandbox.h"

class LinuxSandbox : public Sandbox
{
public:
    LinuxSandbox() = default;

    void run(const UpdateContext& context) {prepare(context); launch(context); }
protected:
    virtual void prepare(const UpdateContext& context) override;
    virtual void launch(const UpdateContext& context) override;
    virtual void cleanup(const UpdateContext& context) override { }

private:
    void copyDependencies(const UpdateContext& context);
    void createRootfs(const UpdateContext& context);

};

#endif // LINUXSANDBOX_H
