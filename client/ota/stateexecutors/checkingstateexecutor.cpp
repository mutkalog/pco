#include "checkingstateexecutor.h"
#include "../../../utils/common/utils.h"

#include "dowloadstateexecutor.h"
#include "idlestateexecutor.h"
#include "../statemachine.h"

void CheckingStateExecutor::execute(StateMachine &sm)
{
    auto&       ctx         = sm.context;
    std::string queryString = httplib::encode_uri(
                                    std::string("/manifest?id=") + std::to_string(ctx.devinfo->id())
                                          + "&type=" + ctx.devinfo->type()
                                          + "&arch=" + ctx.devinfo->arch())
                                          + "&platform=" + ctx.devinfo->platform();
    auto        res          = ctx.client->Get(queryString);

    if (res == nullptr)
    {
        std::cout << "Server unavailable" << std::endl;
        sm.transitTo(&IdleStateExecutor::instance());
        return;
    }
    else if (res->status == httplib::OK_200)
    {
        try
        {
            process(sm, res->body);
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            sm.transitTo(&IdleStateExecutor::instance());
            return;
        }
    }
    else
    {
        std::cout << "Something went wrong. Server retured status: " << res->status << std::endl;
        sm.transitTo(&IdleStateExecutor::instance());
        return;
    }
}

void CheckingStateExecutor::process(StateMachine &sm, const std::string& responseBody)
{
    auto&                ctx              = sm.context;
    json                 data             = json::parse(responseBody);
    std::string          rawManifest      = data["manifest"].get<std::string>();
    json                 signatureInfo    = json::parse(data["signature"].get<std::string>());
    std::string          encodedSignature = signatureInfo["signature"]["value"].get<std::string>();
    std::vector<uint8_t> signature        = SSLUtils::decodeBase64(encodedSignature);

    if (SSLUtils::verifySignature(
            rawManifest, signature,
            ctx.devinfo->publicKeyPath()) == false)
    {
        std::cout << "Signature has not been verified" << std::endl;
        sm.transitTo(&IdleStateExecutor::instance());
        return;
    }

    ctx.manifest.loadFromJson(json::parse(rawManifest));

    if (verificateRelease(ctx.manifest, ctx.devinfo.get()) == true)
    {
        sm.transitTo(&DowloadStateExecutor::instance());
    }
    else
    {
        std::cout << "The latest update already installed." << std::endl;
        ctx.manifest.clear();
        sm.transitTo(&IdleStateExecutor::instance());
    }
}

bool CheckingStateExecutor::verificateRelease(const ArtifactManifest &received, const DeviceInfo *current) const
{
    bool v = compareVersions(received.release.version, current->prevManifest().release.version);
    bool d = compareDeviceType(received, current->prevManifest());

    return v && d;
}

bool CheckingStateExecutor::compareVersions(const std::string &received, const std::string &current) const
{
    if (current.empty()) // first update
    {
        return true;
    }

    return received != current;
}

bool CheckingStateExecutor::compareDeviceType(const ArtifactManifest& received, const ArtifactManifest& current) const
{
    const auto& [lhsType, rhsType] = std::make_tuple(received.release.type,     current.release.type);
    const auto& [lhsOS,     rhsOS] = std::make_tuple(received.release.platform, current.release.platform);
    const auto& [lhsArch, rhsArch] = std::make_tuple(received.release.arch,     current.release.arch);

    if (rhsType.empty() && rhsOS.empty() && rhsArch.empty()) // first update
    {
        return true;
    }

    return lhsType == rhsType && lhsOS == rhsOS && lhsArch == rhsArch;
}
