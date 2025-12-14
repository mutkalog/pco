# find_library(ARCHIVE_LIB archive)

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(ARCHIVE_LIB /usr/lib/x86_64-linux-gnu/libarchive.a)



add_library(client-core
    statemachine.h statemachine.cpp
    stateexecutors/checkingstateexecutor.h stateexecutors/checkingstateexecutor.cpp
    artifactmanifest.h  artifactmanifest.cpp
    stateexecutors/stateexecutor.h stateexecutors/stateexecutor.cpp
    stateexecutors/idlestateexecutor.h stateexecutors/idlestateexecutor.cpp
    stateexecutors/dowloadstateexecutor.h stateexecutors/dowloadstateexecutor.cpp
    stateexecutors/verifyingstateexecutor.h stateexecutors/verifyingstateexecutor.cpp
    stateexecutors/preparingstateexecutor.h stateexecutors/preparingstateexecutor.cpp
    stateexecutors/installingstateexecutor.h stateexecutors/installingstateexecutor.cpp
    deviceinfo.h deviceinfo.cpp
    updatecontext.h updatecontext.cpp
    stateexecutors/finalizingstateexecutor.h stateexecutors/finalizingstateexecutor.cpp
    stateexecutors/registrationexecutor.h stateexecutors/registrationexecutor.cpp
    stateexecutors/committingstateexecutor.h stateexecutors/committingstateexecutor.cpp
    interfaces/ihttpclient.h
    sslhttpclient.h
)


target_compile_definitions(client-core PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

add_executable(pco-client-ota main.cpp)
target_link_libraries(pco-client-ota PUBLIC
    client-core
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

add_subdirectory(tests/client-test)

include(GNUInstallDirs)
install(TARGETS pco-client-ota
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
