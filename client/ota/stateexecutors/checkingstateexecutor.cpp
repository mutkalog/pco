#include "checkingstateexecutor.h"
#include "../../../utils/utils.h"

#include "dowloadstateexecutor.h"
#include "idlestateexecutor.h"
#include "../statemachine.h"

#include <optional>
#include <regex>

void CheckingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    std::string queryString = "/manifest?type=" + ctx.devinfo->type() + "&id=" + std::to_string(ctx.devinfo->id());
    auto res  = ctx.client->Get(queryString);

    if (!res) {
        unsigned long err = ERR_get_error();
        char buf[256] = {0};
        if (err)
            ERR_error_string_n(err, buf, sizeof(buf));
        std::cerr << "httplib Get failed, openssl err: " << buf << "\n";
    }

    json data = json::parse(res->body);

    std::string rawManifest        = data["manifest"].get<std::string>();

    json signatureInfo             = json::parse(data["signature"].get<std::string>());
    std::string encodedSignature   = signatureInfo["signature"]["value"].get<std::string>();
    std::vector<uint8_t> signature = SSLUtils::decodeBase64(encodedSignature);

    if (SSLUtils::verifySignature(
            rawManifest, signature,
            std::string(PROJECT_ROOT_DIR) + "/keys/public.pem") == false)
    {
        std::cout << "Signature has not been verified" << std::endl;
        sm.transitTo(&IdleStateExecutor::instance());
        return;
    }

    try
    {
        ctx.manifest.loadFromJson(json::parse(rawManifest));
    }
    catch (...)
    {
        std::cout << "JSON parsing error" << std::endl;
        sm.transitTo(&IdleStateExecutor::instance());
        return;
    }

    if (verificateRelease(ctx.manifest, ctx.devinfo.get()) == true)
    {
        if (terminateProcesses(ctx))
        {
            sm.transitTo(&DowloadStateExecutor::instance());
        }
        else
        {
            std::cout << "Cannot kill processes" << std::endl;
            sm.transitTo(&IdleStateExecutor::instance());
        }
    }
    else
    {
        std::cout << "The latest update already installed." << std::endl;
        sm.transitTo(&IdleStateExecutor::instance());
    }
}

bool CheckingStateExecutor::verificateRelease(const ArtifactManifest &received, const DeviceInfo *current) const
{
    bool v = compareVersions(received.release.version, current->prevManifest().release.version);
    bool t = compareTimestamps(received.release.timestamp, current->prevManifest().release.timestamp);
    bool d = compareDeviceType(received.release.device, current->type());

    return v && t && d;
}

bool CheckingStateExecutor::compareVersions(const std::string &received, const std::string &current) const
{
    auto split = [](const std::string& version) -> std::optional<std::vector<int>> {
        std::vector<int>  out;
        std::stringstream ss(version);
        std::string       item;

        while (std::getline(ss, item, '.'))
        {
            try
            {
                out.push_back(std::stoi(item));
            }
            catch (...)
            {
                std::cout << "Cannot parse version" << std::endl;
                return std::nullopt;
            }
        }

        return std::make_optional(out);
    };

    std::regex  pattern(R"([0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3})");
    std::smatch match;
    if (std::regex_match(received, match, pattern) == false)
    {
        return false;
    }

    if (current.empty()) // первое обновление
    {
        return true;
    }

    auto recv = split(received);
    auto prev = split(current);

    if (recv.has_value() == false || prev.has_value() == false)
        return false;

    const size_t OCTETS_COUNT = 3;

    for (size_t i = 0; i != OCTETS_COUNT; ++i)
    {
        if (recv.value()[i] > prev.value()[i])
            return true;
    }

    return false;
}

bool CheckingStateExecutor::compareTimestamps(const tm &received, const tm &current) const
{
    tm newT  = received;
    tm prevT = current;

    time_t t1 = mktime(&newT);
    time_t t2 = mktime(&prevT);

    if (t2 == 0) // первое обновление
        return true;

    return t1 > t2;
}

bool CheckingStateExecutor::compareDeviceType(const std::string &received, const std::string &current) const
{
    return received == current;
}

bool CheckingStateExecutor::terminateProcesses(UpdateContext &ctx)
{
    std::cout << "ПРОИСХОДИТ УБИЙСТВО ПРОЦЕССОВ!!!" << std::endl;
    ctx.pm->terminateAll(2000);

    while (ctx.supervisorMq->empty() == false)
    {
        std::cout << "Queue is not empty!" << std::endl;
        auto infoOpt = ctx.supervisorMq->pop();
    }

    return ctx.pm->listChildren().empty() == true;
}

