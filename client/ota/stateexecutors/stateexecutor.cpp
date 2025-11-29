#include "stateexecutor.h"

#include <assert.h>

std::unordered_map<StateExecutor::StateId, std::string>
    StateExecutor::idToNameMap_ = [](){
        std::unordered_map<StateExecutor::StateId, std::string> map = {
            {IDLE,     "IDLE"},
            {CHECKING, "CHECKING"},
            {DOWNLOADING, "DOWNLOADING"},
            {VERIFYING, "VERIFYING"},
            {BUILDING_ENVIRONMENT, "BUILDING_ENVIRONMENT"},
            {TESTING, "TESTING"},
            {COMMITING, "COMMITING"}
        };

        assert(map.size() == StateId::TOTAL);

        return map;
    }();
