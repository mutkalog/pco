#include "core/stateexecutors/installingstateexecutor.h"
#include "core/statemachine.h"
#include "core/updatecontext.h"


void InstallingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    try
    {
        auto& rbMap = ctx.pathToRollbackPathMap;

        for (const auto &file : ctx.devconf->prevManifest().files)
        {
            std::cout << "Creating rollbacks..." << std::endl;
            const auto parentpath = fs::path(file.installPath).parent_path();
            const auto filename   = fs::path(file.installPath).filename();

            if (file.isScript == true)
                continue;

            auto pathToRollbackPath = createRollback(file.installPath);
            std::cout << "Created rollback for " << pathToRollbackPath.first.string()
                      << " to " << pathToRollbackPath.second.string() << std::endl;
            rbMap.insert(std::move(pathToRollbackPath));

            ctx.busyResources.rollbacks = 1;
        }

        if (ctx.devconf->prevManifest().files.empty() == false)
        {
            auto pathToRollbackPath = createRollback(ctx.prevManifestPath);
            std::cout << "Created rollback for " << pathToRollbackPath.first.string()
                      << " to " << pathToRollbackPath.second.string() << std::endl;
            rbMap.insert(std::move(pathToRollbackPath));
        }

        std::cout << "Installing new software..." << std::endl;
        for (const auto &file : ctx.manifest.files)
        {
            const auto parentpath = fs::path(file.installPath).parent_path();
            const auto filename   = fs::path(file.installPath).filename();

            if (file.isScript == true)
                continue;

            installAtomic(ctx.stagingDir / filename, file.installPath);
        }

        createNewArtifatctsPathsVar(ctx);

        std::cout << "Update was successfully installed!" << std::endl;
        sm.transitTo(COMMITTING);
    }
    catch (const std::exception& ex)
    {
        ctx.rollback = true;
        std::cout << ex.what() << std::endl;
        sm.transitTo(FINALIZING);
    }
}

void InstallingStateExecutor::createNewArtifatctsPathsVar(const UpdateContext& ctx)
{
    std::string pcoNewArtifactsPaths;
    for (const auto& file : ctx.manifest.files)
    {
        if (file.isScript == false)
        {
            pcoNewArtifactsPaths += file.installPath.string() + ":";
        }
    }

    pcoNewArtifactsPaths.pop_back();

    std::cout << "PCO_NEW_ARTIFACTS_PATHS: " << pcoNewArtifactsPaths << std::endl;
    setEnvVar("PCO_NEW_ARTIFACTS_PATHS", pcoNewArtifactsPaths);
}


void InstallingStateExecutor::installAtomic(const fs::path &srcStaging, const fs::path &destPath)
{
    const fs::path    parent   = destPath.parent_path();
    const std::string filename = destPath.filename().string();
    const fs::path    rollback = parent / "rollback" / filename;

    fs::create_directories(parent);
    fs::create_directories(rollback.parent_path());

    if (fs::exists(rollback) == false)
    {
        std::cout << "ROLLBACK NOT EXISTS FOR  " << destPath.string()
                  << std::endl;
    }

    fs::path tmp = parent / ("." + filename + ".tmp");
    std::error_code ec;
    fs::copy_file(srcStaging, tmp, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        throw std::system_error(ec, "cannot copy_file srcStaging to tmp: " +
                                        srcStaging.string() + " " + tmp.string());
    }

    int fd = ::open(tmp.c_str(), O_RDONLY);
    if (fd < 0)
    {
        std::cout << "CANNOT open " << tmp.string() << std::endl;
        throw std::system_error(ec, "cannot open tmp fd");
    }

    if (::fsync(fd) != 0)
    {
        std::cout << "CANNOT fsync " << tmp.string() << std::endl;
        ::close(fd);
        ///@todo тут нет ec
        throw std::system_error(ec, "cannot open fsync fd");
    }
    ::close(fd);

    fs::rename(tmp, destPath, ec);
    if (ec)
    {
        std::cout << "CANNOT rename tmp to destPath" << std::endl;
        throw std::system_error(ec, "cannot rename tmp to destPath");
    }

    int dirFd = ::open(parent.c_str(), O_DIRECTORY);
    if (dirFd >= 0)
    {
        ::fsync(dirFd);
        ::close(dirFd);
    }
}


std::pair<fs::path, fs::path>
InstallingStateExecutor::createRollback(const fs::path &file)
{
    const fs::path    parent   = file.parent_path();
    const std::string filename = file.filename().string();
    const fs::path    rollback = parent / "rollback" / filename;

    fs::create_directories(parent);
    fs::create_directories(rollback.parent_path());

    std::error_code ec;

    if (fs::exists(rollback, ec))
    {
        return {file, rollback};
    }

    if (fs::exists(file, ec) == false)
    {
        throw std::system_error(ec, "there is no " + file.string());
    }

    if (!fs::create_directories(rollback.parent_path(), ec) && !fs::exists(rollback.parent_path(), ec))
        throw std::system_error(ec, "cannot create rollback directory ");

    fs::rename(file, rollback, ec);
    if (ec)
    {
        throw std::system_error(ec, "cannot rename " + file.string() +
                                        " to " + rollback.string());
    }

    int dirFd = ::open(parent.c_str(), O_DIRECTORY);
    if (dirFd >= 0)
    {
        ::fsync(dirFd);
        ::close(dirFd);
    }

    return std::pair<fs::path, fs::path>({file, rollback});
}
