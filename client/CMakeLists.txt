
# set(CMAKE_EXE_LINKER_FLAGS "-static")

find_library(ARCHIVE_LIB archive)

# Если установил в /usr/local (классика):
# find_library(ARCHIVE_LIB archive )


# set(ARCHIVE_LIB /usr/lib/x86_64-linux-gnu/libarchive.a)
include_directories("/home/alexander/Projects/posix-compatable-ota/cpp-httplib")

add_executable(pco-client main.cpp
    statemachine.h statemachine.cpp
    updater.h updater.cpp
    archive.h archive.cpp
    data.h)


target_link_libraries(pco-client PRIVATE
    # ${HTTPLIB}
    ${ARCHIVE_LIB}
    mycrypto
    z
    bz2
    lzma
    zstd
    lz4
    crypto
    acl
)

include(GNUInstallDirs)
install(TARGETS pco-client
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
