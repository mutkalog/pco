#ifndef SANDBOX_H
#define SANDBOX_H

#include "../statemachine.h"

class Sandbox
{
public:
    virtual ~Sandbox() = default;

protected:
    virtual void prepare(const UpdateContext& context) = 0;
    virtual void cleanup(const UpdateContext& context) = 0;
    virtual void launch(const UpdateContext& context) = 0;
};

#endif // SANDBOX_H
