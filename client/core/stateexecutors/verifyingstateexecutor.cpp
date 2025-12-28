#include "verifyingstateexecutor.h"

void VerifyingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    try
    {
        std::string pcoStagingArtifactsPaths;
        for (const auto& file : ctx.manifest.files)
        {
            fs::path fileName = fs::path(ctx.stagingDir) / fs::path(file.installPath).filename();

            auto fileHash     = ctx.cryptoUtils->sha256FromFile(fileName);
            auto manifestHash = file.hash.value;

            verifyHashes(fileHash, manifestHash);

            if (file.isScript == false)
            {
                pcoStagingArtifactsPaths  += fileName.string() + ":";
            }
        }

        pcoStagingArtifactsPaths.pop_back();

        std::cout << "PCO_STAGING_ARTIFACTS_PATHS: "  << pcoStagingArtifactsPaths << std::endl;
        setEnvVar("PCO_STAGING_ARTIFACTS_PATHS", pcoStagingArtifactsPaths);

        sm.transitTo(PREPARING);
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

        sm.transitTo(FINALIZING);
    }
}

void VerifyingStateExecutor::verifyHashes(const hash_t &lhs, const hash_t &rhs) const
{
    if (lhs.size() != rhs.size())
        throw std::runtime_error("Hash sizes are not equal");

    for (size_t i = 0; i != lhs.size(); ++i)
    {
        if (lhs[i] != rhs[i])
            throw std::runtime_error("Hashes are not equal");
    }
}

