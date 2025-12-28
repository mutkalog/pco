#ifndef ISTATEPERSISTANCE_H
#define ISTATEPERSISTANCE_H

#include "core/stateexecutors/stateexecutor.h"
#include "core/updatecontext.h"
#include <optional>


class IStatePersistence
{
public:
    virtual ~IStatePersistence() = default;

    virtual void dump(uint32_t state, const UpdateContext& ctx) = 0;
    virtual std::optional<uint32_t> load(UpdateContext& ctx) = 0;
    virtual void clear() = 0;
};

#endif // ISTATEPERSISTANCE_H
