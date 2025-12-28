#ifndef ARCHIVETOOLSADAPTER_H
#define ARCHIVETOOLSADAPTER_H

#include "interfaces/iarchivetools.h"
#include "archivetools.h"


class ArchiveToolsAdapter : public IArchiveTools
{
public:
    int extract(const char* filename, const char* outdir) override
    {
        return ArchiveTools::extract(filename, outdir);
    }

    int extract(const uint8_t* buffer, size_t size, const char* outdir) override
    {
        return ArchiveTools::extract(buffer, size, outdir);
    }

    int create_archive_from_paths(const std::vector<std::string>& paths, std::vector<uint8_t>& out) override
    {
        return ArchiveTools::create_archive_from_paths(paths, out);
    }
};
#endif // ARCHIVETOOLSADAPTER_H
