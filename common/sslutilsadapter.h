#ifndef SSLUTILSADAPTER_H
#define SSLUTILSADAPTER_H

#include "interfaces/icryptoutils.h"
#include "sslutils.h"


class SSLUtilsAdapter : public ISSLUtils
{
public:
    std::vector<uint8_t> decodeBase64(const std::string& input) override
    {
        return SSLUtils::decodeBase64(input);
    }

    std::string encodeBase64(const std::vector<uint8_t>& input) override
    {
        return SSLUtils::encodeBase64(input);
    }

    bool verifySignature(const std::string& manifestData,
                         const std::vector<uint8_t>& signature,
                         const std::string& pubkeyFile) override
    {
        return SSLUtils::verifySignature(manifestData, signature, pubkeyFile);
    }

    std::vector<uint8_t> sha256FromFile(const std::string &path) override
    {
        return SSLUtils::sha256FromFile(path);
    }
};

#endif // SSLUTILSADAPTER_H
