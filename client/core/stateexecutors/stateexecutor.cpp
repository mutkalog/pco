#include "stateexecutor.h"

#include <assert.h>
#include <system_error>

std::unordered_map<StateExecutor::StateId, std::string>
    StateExecutor::idToNameMap_ = [](){
        std::unordered_map<StateExecutor::StateId, std::string> map = {
            { REGISTRATION,   "REGISTRATION" },
            { IDLE,           "IDLE"         },
            { CHECKING,       "CHECKING"     },
            { DOWNLOADING,    "DOWNLOADING"  },
            { VERIFYING,      "VERIFYING"    },
            { PREPARING,      "PREPARING"    },
            { INSTALLING,     "INSTALLING"   },
            { COMMITTING,     "COMMITTING"   },
            { FINALIZING,     "FINALIZING"   }
        };

        assert(map.size() == StateId::TOTAL);

        return map;
    }();

void StateExecutor::setEnvVar(const std::string &key, const std::string &value) const
{
    if (setenv(key.c_str(), value.c_str(), 1) != 0)
        throw std::system_error(std::error_code(errno, std::generic_category()), "Cannot setenv");
}
