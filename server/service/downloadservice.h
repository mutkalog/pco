#ifndef DOWNLOADSERVICE_H
#define DOWNLOADSERVICE_H

#include <cstdint>
#include <string>
#include <vector>
class DownloadService
{
public:
    DownloadService() = default;
    std::vector<uint8_t> getArchive(uint32_t devId, const std::string &devType,
                                        const std::string &platform, const std::string &arch);
};

#endif // DOWNLOADSERVICE_H
