cmake_minimum_required(VERSION 3.16)

project(utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_library(ARCHIVE_LIB archive)

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(ARCHIVE_LIB /usr/lib/x86_64-linux-gnu/libarchive.a)
# include_directories("/home/alexander/Projects/posix-compatable-ota/cpp-httplib")


add_library(common STATIC
  common/utils.cpp
  common/utils.h
  common/archivetools.h common/archivetools.cpp
  common/messagequeue.h
  common/task.h
)

target_link_libraries(common PRIVATE
    ${ARCHIVE_LIB}
    z
    bz2
    lzma
    zstd
    lz4
    ssl
    crypto
    acl
    gmp
    nettle
    hogweed
    xml2
)

target_link_options(common PRIVATE -static)

target_compile_definitions(common PRIVATE UTILS_LIBRARY)
