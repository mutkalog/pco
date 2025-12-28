#ifndef ARCHIVETOOLS_MOCK_H
#define ARCHIVETOOLS_MOCK_H

#include <gmock/gmock.h>
#include "interfaces/iarchivetools.h"

class MockArchiveTools : public IArchiveTools
{
public:
    MOCK_METHOD(int, extract, (const char* filename, const char* outdir), (override));
    MOCK_METHOD(int, extract, (const uint8_t* buffer, size_t size, const char* outdir), (override));
    MOCK_METHOD(int, create_archive_from_paths, (const std::vector<std::string>& paths, std::vector<uint8_t>& out), (override));
};

#endif // ARCHIVETOOLS_MOCK_H
