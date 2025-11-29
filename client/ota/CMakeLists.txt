
# set(CMAKE_EXE_LINKER_FLAGS "-static")

find_library(ARCHIVE_LIB archive)

# Если установил в /usr/local (классика):
# find_library(ARCHIVE_LIB archive )


# set(ARCHIVE_LIB /usr/lib/x86_64-linux-gnu/libarchive.a)
# include_directories("/home/alexander/Projects/posix-compatable-ota/cpp-httplib")

add_executable(pco-client-ota ota/main.cpp
    ota/statemachine.h ota/statemachine.cpp
    ota/stateexecutors/checkingstateexecutor.h ota/stateexecutors/checkingstateexecutor.cpp
    ota/archive.h ota/archive.cpp
    artifactmanifest.h
    ota/stateexecutors/stateexecutor.h ota/stateexecutors/stateexecutor.cpp
    ota/stateexecutors/idlestateexecutor.h ota/stateexecutors/idlestateexecutor.cpp
    ota/stateexecutors/dowloadstateexecutor.h ota/stateexecutors/dowloadstateexecutor.cpp
    ota/stateexecutors/verifyingstateexecutor.h ota/stateexecutors/verifyingstateexecutor.cpp
    ota/stateexecutors/environmentbuildingstateexecutor.h ota/stateexecutors/environmentbuildingstateexecutor.cpp
    ota/sandboxes/sandbox.cpp ota/sandboxes/sandbox.h
    ota/sandboxes/linuxsandbox.h ota/sandboxes/linuxsandbox.cpp
    ota/sandboxes/linuxsandboxinspecor.h ota/sandboxes/linuxsandboxinspecor.cpp
    ota/stateexecutors/testingstateexecutor.h ota/stateexecutors/testingstateexecutor.cpp
    ota/stateexecutors/commitingstateexecutor.h ota/stateexecutors/commitingstateexecutor.cpp
    ota/deviceinfo.h ota/deviceinfo.cpp
    ota/artifactmanifest.cpp
    ota/updatecontext.h ota/updatecontext.cpp)


target_link_libraries(pco-client-ota PRIVATE
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

add_executable(pco-client-runner main.cpp
    statemachine.h statemachine.cpp
    stateexecutors/checkingstateexecutor.h stateexecutors/checkingstateexecutor.cpp
    archive.h archive.cpp
    artifactmanifest.h
    stateexecutors/stateexecutor.h stateexecutors/stateexecutor.cpp
    stateexecutors/idlestateexecutor.h stateexecutors/idlestateexecutor.cpp
    stateexecutors/dowloadstateexecutor.h stateexecutors/dowloadstateexecutor.cpp
    stateexecutors/verifyingstateexecutor.h stateexecutors/verifyingstateexecutor.cpp
    stateexecutors/environmentbuildingstateexecutor.h stateexecutors/environmentbuildingstateexecutor.cpp
    sandboxes/sandbox.cpp sandboxes/sandbox.h
    sandboxes/linuxsandbox.h sandboxes/linuxsandbox.cpp
    sandboxes/linuxsandboxinspecor.h sandboxes/linuxsandboxinspecor.cpp
    stateexecutors/testingstateexecutor.h stateexecutors/testingstateexecutor.cpp
    stateexecutors/commitingstateexecutor.h stateexecutors/commitingstateexecutor.cpp
    deviceinfo.h deviceinfo.cpp
    artifactmanifest.cpp
    updatecontext.h updatecontext.cpp)

include(GNUInstallDirs)
install(TARGETS pco-client-ota
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
