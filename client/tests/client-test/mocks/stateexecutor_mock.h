#ifndef STATEEXECUTOR_MOCK_H
#define STATEEXECUTOR_MOCK_H

#include <gmock/gmock.h>
#include "core/interfaces/istateexecutor.h"


class MockStateExecutor : public IStateExecutor
{
public:
    MOCK_METHOD(void, execute, (StateMachine& sm), (override));
    MOCK_METHOD(uint32_t, id, (), (const, override));
    MOCK_METHOD(std::string, textId, (), (const, override));
};

#endif // STATEEXECUTOR_MOCK_H
