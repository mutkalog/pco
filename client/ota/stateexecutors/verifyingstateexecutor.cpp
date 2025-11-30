#include "verifyingstateexecutor.h"
#include "environmentbuildingstateexecutor.h"
#include "finalizingstateexecutor.h"
#include "../../../utils/utils.h"

void VerifyingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    using hash_t = std::vector<uint8_t>;

    auto isHashsEquals = [](const hash_t& lhs, const hash_t& rhs)
    {
        if (lhs.size() != rhs.size())
            throw std::runtime_error("Hash sizes are not equal");

        for (size_t i = 0; i != lhs.size(); ++i)
        {
            if (lhs[i] != rhs[i])
                return false;
        }

        return true;
    };


    for (const auto& file : ctx.manifest.files)
    {
        fs::path fileName = fs::path(ctx.testingDir) / fs::path(file.installPath).filename();

        auto fileHash        = SSLUtils::sha256FromFile(fileName);
        auto manifestHash    = file.hash.value;

        if (isHashsEquals(fileHash, manifestHash) == false)
        {
            std::string message = "Hashes are not equal";
            std::cout << message << std::endl;
            ctx.reportMessage =
            {
                HASHES_NOT_EQUAL,
                message
            };

            sm.transitTo(&FinalizingStateExecutor::instance());
            return;
        }
    }

    sm.instance().transitTo(&EnvironmentBuildingStateExecutor::instance());
}
