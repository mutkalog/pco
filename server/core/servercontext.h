#ifndef SERVERCONTEXT_H
#define SERVERCONTEXT_H

#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <string>

#include "interfaces/icryptoutils.h"
#include "interfaces/iarchivetools.h"


using entry_id_t = uint64_t;

struct UpdateInfo
{
    uint64_t    releaseId;
    bool        finished;
    uint64_t    status;
    std::string report;
    std::chrono::steady_clock::time_point expireTime;
};

struct Updates
{
    std::mutex mtx;
    std::condition_variable cv;
    std::unordered_map<entry_id_t, UpdateInfo> devToReleaseMap;
};


struct RolloutInfo
{
    std::string type;
    std::string platform;
    std::string arch;
    bool 		 isCanary;
    int 		 inRolloutPercentage;
    int 		 nextSelectionPercentage;
};

struct Rollouts
{
    std::mutex mtx;
    std::condition_variable cv;
    std::unordered_map<entry_id_t, RolloutInfo> releaseToInfoMap;
};


struct Staging
{
    Updates updates;
    Rollouts rollouts;
};

struct ServerContext
{
    Staging staging;
    std::unique_ptr<ISSLUtils> cryptoUtils;
    std::unique_ptr<IArchiveTools> archiveTools;

    ServerContext(std::unique_ptr<ISSLUtils> cu,
                  std::unique_ptr<IArchiveTools> at)
        : cryptoUtils(std::move(cu))
        , archiveTools(std::move(at)) {}
};


#endif // SERVERCONTEXT_H
