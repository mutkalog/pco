

# find_library(ARCHIVE_LIB archive)

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(ARCHIVE_LIB /usr/lib/x86_64-linux-gnu/libarchive.a)


add_executable(pco-client-ota ota/main.cpp
    ota/statemachine.h ota/statemachine.cpp
    ota/stateexecutors/checkingstateexecutor.h ota/stateexecutors/checkingstateexecutor.cpp
    ota/artifactmanifest.h  ota/artifactmanifest.cpp
    ota/stateexecutors/stateexecutor.h ota/stateexecutors/stateexecutor.cpp
    ota/stateexecutors/idlestateexecutor.h ota/stateexecutors/idlestateexecutor.cpp
    ota/stateexecutors/dowloadstateexecutor.h ota/stateexecutors/dowloadstateexecutor.cpp
    ota/stateexecutors/verifyingstateexecutor.h ota/stateexecutors/verifyingstateexecutor.cpp
    ota/stateexecutors/preparingstateexecutor.h ota/stateexecutors/preparingstateexecutor.cpp
    ota/stateexecutors/installingstateexecutor.h ota/stateexecutors/installingstateexecutor.cpp
    ota/deviceinfo.h ota/deviceinfo.cpp
    ota/updatecontext.h ota/updatecontext.cpp
    ota/stateexecutors/finalizingstateexecutor.h ota/stateexecutors/finalizingstateexecutor.cpp
    ota/stateexecutors/registrationexecutor.h ota/stateexecutors/registrationexecutor.cpp
    ota/stateexecutors/committingstateexecutor.h ota/stateexecutors/committingstateexecutor.cpp
)

target_compile_definitions(pco-client-ota PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

target_link_libraries(pco-client-ota PRIVATE
    common
    gmp
    nettle
    hogweed
    xml2
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
