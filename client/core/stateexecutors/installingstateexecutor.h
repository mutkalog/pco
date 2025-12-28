#ifndef INSTALLINGSTATEEXECUTOR_H
#define INSTALLINGSTATEEXECUTOR_H

#include "core/updatecontext.h"
#include "core/stateexecutors/stateexecutor.h"
#include <filesystem>


namespace fs = std::filesystem;

class InstallingStateExecutor : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;
    InstallingStateExecutor(enum StateId id) : StateExecutor(id) {}

protected:
    virtual void createNewArtifatctsPathsVar(const UpdateContext &ctx);
    virtual void installAtomic(const fs::path& srcStaging, const fs::path& destPath);
    virtual std::pair<fs::path, fs::path> createRollback(const fs::path& file);
};

#endif // INSTALLINGSTATEEXECUTOR_H
