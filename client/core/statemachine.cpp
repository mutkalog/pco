#include "statemachine.h"

StateMachine::StateMachine(std::unique_ptr<IStatePersistence> sp,
                           UpdateContext ctx,
                           std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor> > &&map,
                           uint32_t firstStateId,
                           uint32_t criticalStateId)
    : context(std::move(ctx))
    , sp_(std::move(sp))
    , idToStateMap_(std::move(map))
    , criticalStateId_(criticalStateId)
{
    auto it = idToStateMap_.find(firstStateId);
    if (it == idToStateMap_.end())
        throw std::runtime_error("corrupted idToStateMap_");

    currentSE_ = it->second.get();
}

void StateMachine::init()
{
    recover();
    context.updateEnvironmentVars();
}

void StateMachine::transitTo(uint32_t nextState)
{
    auto it = idToStateMap_.find(nextState);
    if (it == idToStateMap_.end())
        return;

    auto executor = it->second.get();

    std::cout << "Transition to " << executor->textId() << " state" << std::endl;
    inCriticalStates_ = (nextState >= criticalStateId_);
    try
    {
        if (inCriticalStates_)
        {
            std::cout << "Writing state " + std::to_string(nextState) + " to state file" << std::endl;
            sp_->dump(nextState, context);
        }
        else
        {
            sp_->clear();
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    currentSE_ = executor;
}

void StateMachine::recover()
{
    try
    {
        auto state = sp_->load(context);
        if (state.has_value())
        {
            std::cout << "Recovering to state " << *state << std::endl;

            auto it = idToStateMap_.find(*state);
            if (it == idToStateMap_.end())
            {
                std::cout << "Cannot find state " << *state <<
                    ". Aborting recovery." << std::endl;
                return;
            }

            currentSE_         = it->second.get();
            inCriticalStates_  = true;
            context.recovering = true;
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << "StateMachine: cannot recover: " << ex.what() << std::endl;
    }
}

