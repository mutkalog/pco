#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <cstddef>
#include <cstdint>

int extract(const char* filename, const char* outdir);
int extract(const uint8_t* buffer, size_t size, const char* outdir);

class Archive
{
public:
    Archive();
};

#endif // ARCHIVE_H
