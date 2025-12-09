#ifndef INSTALLINGSTATEEXECUTOR_H
#define INSTALLINGSTATEEXECUTOR_H

#include "stateexecutor.h"
#include <filesystem>



namespace fs = std::filesystem;

class InstallingStateExecutor final : public StateExecutor
{
public:
    static InstallingStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

private:
    InstallingStateExecutor(enum StateId id) : StateExecutor(id) {}
    void installAtomic(const fs::path& srcStaging, const fs::path& destPath);
    std::pair<fs::path, fs::path> createRollback(const fs::path& file);
};

inline InstallingStateExecutor &InstallingStateExecutor::instance()
{
    static InstallingStateExecutor inst(INSTALLING);
    return inst;
}

#endif // INSTALLINGSTATEEXECUTOR_H
