#ifndef STATEPERSISTENCE_H
#define STATEPERSISTENCE_H

#include "interfaces/istatepersistance.h"


class StatePersistence : public IStatePersistence
{
public:
    StatePersistence(const fs::path& stateFile) : stateFile_(stateFile) {}
    
    virtual void dump(uint32_t state, const UpdateContext& ctx) override;
    virtual std::optional<uint32_t> load(UpdateContext& ctx) override;
    virtual void clear() override;

private:
    fs::path stateFile_;
};

#endif // STATEPERSISTENCE_H
