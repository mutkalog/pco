#include "statepersistence.h"
#include <fstream>
#include "core/stateexecutors/stateexecutor.h"


void StatePersistence::dump(uint32_t state, const UpdateContext &ctx)
{
    json stateAndContext;
    stateAndContext["state"]   = state;
    stateAndContext["context"] = ctx.dumpContext();

    int fd = ::open(stateFile_.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0644);
    if (fd < 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot open state file " + stateFile_.string());
    }

    std::string stringRepr = stateAndContext.dump();

    if (::write(fd, stringRepr.data(), stringRepr.size()) == -1)
    {
        ::close(fd);
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot write to state file");
    }

    ::close(fd);
}

std::optional<uint32_t> StatePersistence::load(UpdateContext &ctx)
{
    std::ifstream stateFile(stateFile_, std::ios_base::in | std::ios_base::binary);

    if (!stateFile)
        return std::nullopt;

    json stateAndContext;
    stateFile >> stateAndContext;

    uint32_t state       = stateAndContext["state"].get<uint32_t>();
    json     contextJson = stateAndContext["context"];

    StateExecutor::StateId result{};
    std::memcpy(&result, &state, sizeof(state));

    ctx.loadContext(contextJson);

    return std::make_optional(result);
}

void StatePersistence::clear()
{
    if (fs::exists(stateFile_))
    {
        std::error_code ec;
        bool            removed = fs::remove(stateFile_, ec);

        if (ec || removed == false)
        {
            throw std::system_error(ec, "cannot rm STATE_FILE");
        }

        int dirFd = ::open(stateFile_.parent_path().c_str(), O_DIRECTORY | O_RDONLY);
        if (dirFd < 0)
        {
            throw std::system_error(
                std::error_code(errno, std::generic_category()),
                "Cannot open state file parent dir " + stateFile_.string());
        }

        if (::fsync(dirFd) != 0)
        {
            ::close(dirFd);
            throw std::system_error(std::error_code(errno, std::generic_category()),
                            "Cannot fsync state file parent dir");
        }

        ::close(dirFd);
     }
}
