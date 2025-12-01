#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <cstddef>
#include <cstdint>

#include <vector>
#include <string>

int extract(const char* filename, const char* outdir);
int extract(const uint8_t* buffer, size_t size, const char* outdir);
int create_archive_from_paths(const std::vector<std::string>& paths, std::vector<uint8_t>& out);

class Archive
{
public:
    Archive();
};

#endif // ARCHIVE_H
