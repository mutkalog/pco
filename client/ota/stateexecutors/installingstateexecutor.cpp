// #include "installingstateexecutor.h"

// void InstallingStateExecutor::execute(StateMachine &sm)
// {
//     // auto& ctx = sm.context;

//     // try
//     // {
//     //     std::ofstream installingFlag("/var/pco/installing-in-process",
//     //                                  std::ios_base::out | std::ios_base::__noreplace);
//     //     if (!installingFlag)
//     //         throw std::runtime_error("Insltalling in process flag already exists");


//     //     std::string pcoStagingArtifactsPaths;
//     //     for (const auto &file : ctx.manifest.files) {
//     //         fs::path fileName =
//     //             fs::path(ctx.stagingDir) / fs::path(file.installPath).filename();

//     //         auto fileHash = SSLUtils::sha256FromFile(fileName);
//     //         auto manifestHash = file.hash.value;

//     //         isHashsEquals(fileHash, manifestHash);
//     //         pcoStagingArtifactsPaths = fileName;
//     //     }

//     //     sm.instance().transitTo(&PreInstallScriptStateExecutor::instance());
//     // }
//     // catch (const std::exception& ex)
//     // {
//     //     std::string message = ex.what();
//     //     std::cout << message << std::endl;
//     //     ctx.reportMessage =
//     //     {
//     //         ARTIFACT_INTEGRITY_ERROR,
//     //         message
//     //     };

//     //     sm.transitTo(&FinalizingStateExecutor::instance());
//     // }
// }
