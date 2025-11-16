#include "archive.h"

Archive::Archive() {}

#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <filesystem>

static int copy_data(struct archive* ar, struct archive* aw) {
    const void* buff;
    size_t size;
    la_int64_t offset;

    for (;;) {
        int r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
            return ARCHIVE_OK;
        if (r < ARCHIVE_OK)
            return r;
        r = archive_write_data_block(aw, buff, size, offset);
        if (r < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(aw));
            return r;
        }
    }
}

int extract(const char* filename, const char* outdir) {
    struct archive* a;
    struct archive* ext;
    struct archive_entry* entry;
    int r;

    a = archive_read_new();
    archive_read_support_filter_gzip(a);
    archive_read_support_format_tar(a);

    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME |
        ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL  |
        ARCHIVE_EXTRACT_FFLAGS);

    if ((r = archive_read_open_filename(a, filename, 10240))) {
        std::cerr << "archive_read_open failed: " << archive_error_string(a) << std::endl;
        return r;
    }

    while (true) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;

        if (r < ARCHIVE_OK)
            std::cerr << archive_error_string(a) << std::endl;

        if (r < ARCHIVE_WARN)
            return r;

        const char* current = archive_entry_pathname(entry);

        // Каталог назначения
        std::string fullpath = std::string(outdir) + "/" + current;

        if (std::filesystem::exists(fullpath) == false)
            throw std::runtime_error("Given archive output path does not exist");

        archive_entry_set_pathname(entry, fullpath.c_str());

        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK)
            std::cerr << archive_error_string(ext) << std::endl;
        else if (archive_entry_size(entry) > 0) {
            r = copy_data(a, ext);
            if (r < ARCHIVE_OK)
                std::cerr << archive_error_string(ext) << std::endl;
        }

        r = archive_write_finish_entry(ext);
        if (r < ARCHIVE_OK)
            std::cerr << archive_error_string(ext) << std::endl;
    }

    archive_read_close(a);
    archive_read_free(a);

    archive_write_close(ext);
    archive_write_free(ext);

    return ARCHIVE_OK;
}

int extract(const uint8_t *buffer, size_t size, const char* outdir) {
    struct archive* a = archive_read_new();
    struct archive* ext = archive_write_disk_new();
    struct archive_entry* entry;

    archive_read_support_format_tar(a);
    archive_read_support_filter_gzip(a);

    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME |
        ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL  |
        ARCHIVE_EXTRACT_FFLAGS);

    if (archive_read_open_memory(a, buffer, size) != ARCHIVE_OK) {
        std::cerr << archive_error_string(a) << "\n";
        return ARCHIVE_FAILED;
    }

    while (true) {
        int r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;

        if (r < ARCHIVE_OK)
            std::cerr << archive_error_string(a) << "\n";
        if (r < ARCHIVE_WARN)
            return r;

        std::string fullpath = std::string(outdir) + "/" + archive_entry_pathname(entry);
        archive_entry_set_pathname(entry, fullpath.c_str());

        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK)
            std::cerr << archive_error_string(ext) << "\n";
        else if (archive_entry_size(entry) > 0) {
            const void* buff;
            size_t size;
            int64_t offset;

            while (true) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF)
                    break;
                if (r < ARCHIVE_OK)
                    return r;

                r = archive_write_data_block(ext, buff, size, offset);
                if (r < ARCHIVE_OK)
                    return r;
            }
        }

        r = archive_write_finish_entry(ext);
        if (r < ARCHIVE_OK)
            std::cerr << archive_error_string(ext) << "\n";
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return ARCHIVE_OK;
}
