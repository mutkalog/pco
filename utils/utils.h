#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <vector>
#include <string>

class SSLUtils {
public:
    static std::vector<uint8_t> decodeBase64(const std::string& input);
    static bool verifySignature(const std::string& manifestData, const std::vector<uint8_t>& signature, const std::string& pubkeyFile);
    static std::vector<uint8_t> sha256FromFile(const std::string &path);

};

#endif // UTILS_H
