#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <cstddef>
#include <cstdint>
#include <archive.h>
#include <archive_entry.h>

#include <vector>
#include <string>


class ArchiveTools
{
public:
    static int extract(const char* filename, const char* outdir);
    static int extract(const uint8_t* buffer, size_t size, const char* outdir);
    static int create_archive_from_paths(const std::vector<std::string>& paths, std::vector<uint8_t>& out);
};


#endif // ARCHIVE_H
