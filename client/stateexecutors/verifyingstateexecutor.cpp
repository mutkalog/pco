#include "verifyingstateexecutor.h"
#include "environmentbuildingstateexecutor.h"
#include "idlestateexecutor.h"
#include "../../utils/utils.h"

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
        auto i = file.installPath.rfind('/');
        if (i == std::string::npos)
            throw std::runtime_error("Invalid install path");

        std::string programName = file.installPath.substr(i + 1);
        std::string fileName    = ctx.testingDir + "/" + programName;

        auto fileHash     = SSLUtils::sha256FromFile(fileName);
        auto manifestHash = file.hash.value;

        if (isHashsEquals(fileHash, manifestHash) == false)
        {
            sm.instance().transitTo(&IdleStateExecutor::instance());
            std::cout << "FAIL" << std::endl;

            break;
        }
    }

    sm.instance().transitTo(&EnvironmentBuildingStateExecutor::instance());

}
