#ifndef ICRYPTOUTILS_H
#define ICRYPTOUTILS_H

#include <cstdint>
#include <vector>
#include <string>

struct ISSLUtils
{
    virtual ~ISSLUtils() = default;

    virtual std::vector<uint8_t> decodeBase64(const std::string& input) = 0;
    virtual std::string encodeBase64(const std::vector<uint8_t>& input) = 0;
    virtual bool verifySignature(const std::string& manifestData,
                                 const std::vector<uint8_t>& signature,
                                 const std::string& pubkeyFile) = 0;
    virtual std::vector<uint8_t> sha256FromFile(const std::string &path) = 0;
};

#endif // ICRYPTOUTILS_H
