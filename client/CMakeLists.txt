
# set(CMAKE_EXE_LINKER_FLAGS "-static")

find_library(ARCHIVE_LIB archive)

# set(ARCHIVE_LIB /usr/lib/x86_64-linux-gnu/libarchive.a)
# include_directories("/home/alexander/Projects/posix-compatable-ota/cpp-httplib")

add_executable(pco-client-ota ota/main.cpp
    ota/statemachine.h ota/statemachine.cpp
    ota/stateexecutors/checkingstateexecutor.h ota/stateexecutors/checkingstateexecutor.cpp
    ota/archive.h ota/archive.cpp
    ota/artifactmanifest.h  ota/artifactmanifest.cpp
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
    ota/updatecontext.h ota/updatecontext.cpp
    ota/processmanager.h ota/processmanager.cpp
    ota/stateexecutors/finalizingstateexecutor.h ota/stateexecutors/finalizingstateexecutor.cpp
    ota/stateexecutors/registrationexecutor.h ota/stateexecutors/registrationexecutor.cpp)

target_compile_definitions(pco-client-ota PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

target_link_libraries(pco-client-ota PRIVATE
    ${ARCHIVE_LIB}
    mycrypto
    z
    bz2
    lzma
    zstd
    lz4
    ssl
    crypto
    acl
)


include(GNUInstallDirs)
install(TARGETS pco-client-ota
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
