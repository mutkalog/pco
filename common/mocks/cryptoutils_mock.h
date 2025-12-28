#ifndef CRYPTOUTILS_MOCK_H
#define CRYPTOUTILS_MOCK_H

#include <gmock/gmock.h>
#include "interfaces/icryptoutils.h"

class MockCryptoUtils : public ISSLUtils
{
public:
    MOCK_METHOD(std::vector<uint8_t>, decodeBase64, (const std::string&), (override));
    MOCK_METHOD(bool, verifySignature, (const std::string&, const std::vector<uint8_t>&, const std::string&), (override));
    MOCK_METHOD(std::vector<uint8_t>, sha256FromFile, (const std::string &path), (override));
};

#endif // CRYPTOUTILS_MOCK_H
