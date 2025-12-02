#include "archive.h"

Archive::Archive() {}

#include <iostream>
#include <filesystem>

#include <fstream>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>

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



namespace fs = std::filesystem;

// write callback для libarchive — дописывает данные в vector<uint8_t>
static la_ssize_t write_callback(struct archive* a, void* client_data, const void* buff, size_t length)
{
    auto out = static_cast<std::vector<uint8_t>*>(client_data);
    const uint8_t* b = static_cast<const uint8_t*>(buff);
    out->insert(out->end(), b, b + length);
    return static_cast<la_ssize_t>(length);
}

// простые no-op open/close callbacks
static int open_callback(struct archive* a, void* client_data) { (void)a; (void)client_data; return ARCHIVE_OK; }
static int close_callback(struct archive* a, void* client_data) { (void)a; (void)client_data; return ARCHIVE_OK; }

int create_archive_from_paths(const std::vector<std::string>& paths, std::vector<uint8_t>& out)
{
    out.clear();

    struct archive* a = archive_write_new();
    if (!a) return ARCHIVE_FATAL;

    // tar format + gzip filter
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a); // совместимый tar

    // Открываем с callback-ами, client_data = &out
    if (archive_write_open(a, &out, open_callback, write_callback, close_callback) != ARCHIVE_OK) {
        archive_write_free(a);
        return ARCHIVE_FATAL;
    }

    for (const auto& path_str : paths) {
        fs::path p(path_str);

        // Пропускаем несуществующие и не-regular файлы
        std::error_code ec;
        if (!fs::exists(p, ec) || !fs::is_regular_file(p, ec)) {
            // можно вернуть ошибку или просто пропустить; здесь возвращаем предупреждение
            archive_write_close(a);
            archive_write_free(a);
            return ARCHIVE_WARN;
        }

        // Получаем метаинфу
        struct stat st;
        if (stat(p.c_str(), &st) != 0) {
            archive_write_close(a);
            archive_write_free(a);
            return ARCHIVE_WARN;
        }

        // Создаём запись
        struct archive_entry* entry = archive_entry_new();
        if (!entry) {
            archive_write_close(a);
            archive_write_free(a);
            return ARCHIVE_FATAL;
        }

        // Имя внутри архива — только filename. Заменить на p.string() если хотите сохранить путь.
        std::string entry_name = p.filename().string();
        archive_entry_set_pathname(entry, entry_name.c_str());
        archive_entry_set_size(entry, static_cast<la_int64_t>(st.st_size));
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, st.st_mode & 0777);
        archive_entry_set_mtime(entry, st.st_mtime, 0);

        int r = archive_write_header(a, entry);
        if (r < ARCHIVE_OK) {
            // ошибка заголовка
            archive_entry_free(entry);
            archive_write_close(a);
            archive_write_free(a);
            return r;
        }

        // Если есть содержимое — читаем и пишем
        if (st.st_size > 0) {
            std::ifstream ifs(p, std::ios::binary);
            if (!ifs) {
                archive_entry_free(entry);
                archive_write_close(a);
                archive_write_free(a);
                return ARCHIVE_WARN;
            }

            const size_t BUF_SZ = 64 * 1024;
            std::vector<char> buf(BUF_SZ);
            while (ifs) {
                ifs.read(buf.data(), static_cast<std::streamsize>(buf.size()));
                std::streamsize readn = ifs.gcount();
                if (readn > 0) {
                    la_ssize_t wrote = archive_write_data(a, buf.data(), static_cast<size_t>(readn));
                    if (wrote < 0) {
                        ifs.close();
                        archive_entry_free(entry);
                        archive_write_close(a);
                        archive_write_free(a);
                        return static_cast<int>(wrote);
                    }
                }
            }
        }

        r = archive_write_finish_entry(a);
        archive_entry_free(entry);
        if (r < ARCHIVE_OK) {
            archive_write_close(a);
            archive_write_free(a);
            return r;
        }
    }

    // Завершаем архив
    archive_write_close(a);
    archive_write_free(a);

    return ARCHIVE_OK; // out содержит готовый gzipped tar
}
