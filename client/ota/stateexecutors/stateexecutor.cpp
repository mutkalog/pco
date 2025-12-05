#include "stateexecutor.h"

#include <assert.h>

std::unordered_map<StateExecutor::StateId, std::string>
    StateExecutor::idToNameMap_ = [](){
        std::unordered_map<StateExecutor::StateId, std::string> map = {
            { REGISTRATION,   "REGISTRATION"   },
            { IDLE,           "IDLE"           },
            { CHECKING,       "CHECKING"       },
            { DOWNLOADING,    "DOWNLOADING"    },
            { VERIFYING,      "VERIFYING"      },
            { PREPARING,      "PREPARING"      },
            { PRE_COMMITTING, "PRE_COMMITTING" },
            { COMMITTING,     "COMMITING"      },
            { FINALIZING,     "FINALIZING"     }
        };

        assert(map.size() == StateId::TOTAL);

        return map;
    }();
