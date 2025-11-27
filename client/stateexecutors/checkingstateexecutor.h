#ifndef CHECKINGSTATEEXECUTOR_H
#define CHECKINGSTATEEXECUTOR_H

#include <httplib.h>
#include "../artifactmanifest.h"
#include "stateexecutor.h"
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

class CheckingStateExecutor final : public StateExecutor
{
public:
    static CheckingStateExecutor& instance();

    virtual void execute(StateMachine& sm) override;

private:
    /// @todo: сделать файл конфиг на клиенте, хранящий настройки процедуры обновления:
    /// интервал полинга, урл сервера и т.д.
    CheckingStateExecutor(enum StateId id) : StateExecutor(id) {}

    void loadManifestFromJson(ArtifactManifest &manifest,
                              const nlohmann::json &data);

    std::vector<uint8_t> parseHashFromString(const std::string& stringHash);
};

inline CheckingStateExecutor &CheckingStateExecutor::instance()
{
    static CheckingStateExecutor inst(CHECKING);
    return inst;
}

#endif // CHECKINGSTATEEXECUTOR_H
