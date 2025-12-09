#include "verifyingstateexecutor.h"
#include "preparingstateexecutor.h"
#include "finalizingstateexecutor.h"
#include "../../../utils/common/utils.h"

void VerifyingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    using hash_t = std::vector<uint8_t>;

    auto isHashsEquals = [](const hash_t& lhs, const hash_t& rhs) -> void
    {
        if (lhs.size() != rhs.size())
            throw std::runtime_error("Hash sizes are not equal");

        for (size_t i = 0; i != lhs.size(); ++i)
        {
            if (lhs[i] != rhs[i])
                throw std::runtime_error("Hashes are not equal");
        }
    };

    try
    {
        std::string pcoStagingArtifactsPaths;
        std::string pcoRequiredArtifactsPaths;
        for (const auto& file : ctx.manifest.files)
        {
            fs::path fileName = fs::path(ctx.stagingDir) / fs::path(file.installPath).filename();

            auto fileHash        = SSLUtils::sha256FromFile(fileName);
            auto manifestHash    = file.hash.value;

            isHashsEquals(fileHash, manifestHash);

            if (file.installPath.parent_path().empty() == false)
            {
                pcoStagingArtifactsPaths += fileName.string() + ":";
                pcoRequiredArtifactsPaths += file.installPath.string() + ":";
            }
        }

        pcoStagingArtifactsPaths  = pcoStagingArtifactsPaths.substr(0, pcoStagingArtifactsPaths.size() - 1);
        pcoRequiredArtifactsPaths = pcoRequiredArtifactsPaths.substr(0, pcoRequiredArtifactsPaths.size() - 1);
        std::cout << "PCO_STAGING_ARTIFACTS_PATHS: "  << pcoStagingArtifactsPaths  << std::endl;
        std::cout << "PCO_REQUIRED_ARTIFACTS_PATHS: " << pcoRequiredArtifactsPaths << std::endl;

        if (setenv("PCO_STAGING_ARTIFACTS_PATHS", pcoStagingArtifactsPaths.c_str(), 1) != 0)
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                    "Cannot setenv PCO_STAGING_ARTIFACTS_PATHS");
        }

        if (setenv("PCO_REQUIRED_ARTIFACTS_PATHS", pcoRequiredArtifactsPaths.c_str(), 1) != 0)
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                    "Cannot setenv PCO_REQUIRED_ARTIFACTS_PATHS");
        }

        sm.instance().transitTo(&PreparingStateExecutor::instance());
    }
    catch (const std::exception& ex)
    {
        std::string message = ex.what();
        std::cout << message << std::endl;
        ctx.rollback = true;
        ctx.reportMessage =
        {
            ARTIFACT_INTEGRITY_ERROR,
            message
        };

        sm.transitTo(&FinalizingStateExecutor::instance());
    }
}
