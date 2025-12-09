#include "statemachine.h"
#include "stateexecutors/registrationexecutor.h"

#include <fstream>

#include <ota/stateexecutors/registrationexecutor.h>
#include <ota/stateexecutors/idlestateexecutor.h>
#include <ota/stateexecutors/checkingstateexecutor.h>
#include <ota/stateexecutors/verifyingstateexecutor.h>
#include <ota/stateexecutors/dowloadstateexecutor.h>
#include <ota/stateexecutors/preparingstateexecutor.h>
#include <ota/stateexecutors/installingstateexecutor.h>
#include <ota/stateexecutors/committingstateexecutor.h>
#include <ota/stateexecutors/finalizingstateexecutor.h>

namespace {
const fs::path STATE_FILE = "/var/pco/state.json";
}

StateExecutor* StateMachine::stateTable_[] =
{
    &RegistrationExecutor::instance(),
    &IdleStateExecutor::instance(),
    &CheckingStateExecutor::instance(),
    &DowloadStateExecutor::instance(),
    &VerifyingStateExecutor::instance(),
    &PreparingStateExecutor::instance(),
    &InstallingStateExecutor::instance(),
    &CommittingStateExecutor::instance(),
    &FinalizingStateExecutor::instance(),
};


StateMachine::StateMachine()
    : currentSE_(&RegistrationExecutor::instance())
{
    recover();
}

void StateMachine::recover()
{
    try
    {
        auto state = loadMachineState();
        if (state.has_value())
        {
            std::cout << "Recovering to state " << *state << std::endl;

            currentSE_         = stateTable_[*state];
            inCriticalStates_  = true;
            context.recovering = true;
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << "StateMachine: cannot recover: " << ex.what() << std::endl;
    }
}

void StateMachine::dumpMachineState(StateExecutor::StateId state)
{
    json stateAndContext;
    stateAndContext["state"]   = state;
    stateAndContext["context"] = context.dumpContext();

    int fd = ::open(STATE_FILE.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot open state file " + STATE_FILE.string());
    }

    std::string stringRepr = stateAndContext.dump();

    if (::write(fd, stringRepr.data(), stringRepr.size()) == -1)
    {
        ::close(fd);
        throw std::system_error(std::error_code(errno, std::generic_category()),
                        "Cannot write to state file");
    }

    if (::fsync(fd) != 0)
    {
        ::close(fd);
        throw std::system_error(std::error_code(errno, std::generic_category()),
                        "Cannot fsync state file");
    }

    ::close(fd);
}

std::optional<StateExecutor::StateId> StateMachine::loadMachineState()
{
    std::ifstream stateFile(STATE_FILE, std::ios_base::in | std::ios_base::binary);

    if (!stateFile)
        return std::nullopt;

    json stateAndContext;
    stateFile >> stateAndContext;

    size_t state       = stateAndContext["state"].get<size_t>();
    json   contextJson = stateAndContext["context"];

    StateExecutor::StateId result{};
    std::memcpy(&result, &state, sizeof(state));

    context.loadContext(contextJson);

    return std::make_optional(result);
}


void StateMachine::transitTo(StateExecutor *se)
{
    std::cout << "Transition to " << se->textId() << " state" << std::endl;
    auto nextState = se->id();

    inCriticalStates_ = (nextState > StateExecutor::DOWNLOADING);

    try
    {
        if (inCriticalStates_)
        {
            std::cout << "Writing state " + std::to_string(nextState) + " to state file" << std::endl;
            dumpMachineState(nextState);
        }
        else if (fs::exists(STATE_FILE))
        {
            fs::remove_all(STATE_FILE);
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    currentSE_ = se;
}
