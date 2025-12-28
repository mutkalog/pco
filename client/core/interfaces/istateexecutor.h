#ifndef ISTATEEXECUTOR_H
#define ISTATEEXECUTOR_H

#include <cstdint>
#include <string>


class StateMachine;

class IStateExecutor
{
public:
    virtual ~IStateExecutor() = default;
    virtual void execute(StateMachine& sm) = 0;
    virtual uint32_t id() const = 0;
    virtual std::string textId() const = 0;
};

#endif // ISTATEEXECUTOR_H
