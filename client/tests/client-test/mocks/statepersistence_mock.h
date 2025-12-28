#ifndef STATEPERSISTENCE_MOCK_H
#define STATEPERSISTENCE_MOCK_H

#include <gmock/gmock.h>
#include "core/interfaces/istatepersistance.h"


class MockStatePersistence : public IStatePersistence
{
public:
    MOCK_METHOD(void, dump, (uint32_t state, const UpdateContext& ctx), (override));
    MOCK_METHOD(std::optional<uint32_t>, load, (UpdateContext& ctx), (override));
    MOCK_METHOD(void, clear, (), (override));
};

#endif // STATEPERSISTENCE_MOCK_H
