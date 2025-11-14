#include "archive.h"

Archive::Archive() {}

#include <archive.h>
#include <archive_entry.h>
#include <iostream>

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
