#ifndef SANDBOX_H
#define SANDBOX_H


class UpdateContext;

class Sandbox
{
public:
    virtual ~Sandbox() = default;

    virtual void prepare(UpdateContext& context) = 0;
    virtual void cleanup(UpdateContext& context) = 0;
    virtual void launch(UpdateContext& context) = 0;
};

class SandboxInspector
{
public:
    virtual ~SandboxInspector() = default;

    virtual void inspect(UpdateContext& context) = 0;
    virtual void cleanup(UpdateContext& context) = 0;
};

#endif // SANDBOX_H
