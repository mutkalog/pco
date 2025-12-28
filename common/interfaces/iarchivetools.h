#ifndef IARCHIVETOOLS_H
#define IARCHIVETOOLS_H

#include <cstdint>
#include <vector>
#include <string>


class IArchiveTools
{
public:
    virtual ~IArchiveTools() = default;

    virtual int extract(const char* filename, const char* outdir) = 0;
    virtual int extract(const uint8_t* buffer, size_t size, const char* outdir) = 0;
    virtual int create_archive_from_paths(const std::vector<std::string>& paths, std::vector<uint8_t>& out) = 0;
};

#endif // IARCHIVETOOLS_H
