add_library(client-core
    core/statemachine.h
    core/statemachine.cpp
    core/stateexecutors/checkingstateexecutor.h
    core/stateexecutors/checkingstateexecutor.cpp
    core/artifactmanifest.h
    core/artifactmanifest.cpp
    core/stateexecutors/stateexecutor.h
    core/stateexecutors/stateexecutor.cpp
    core/stateexecutors/idlestateexecutor.h
    core/stateexecutors/idlestateexecutor.cpp
    core/stateexecutors/dowloadingstateexecutor.h
    core/stateexecutors/dowloadingstateexecutor.cpp
    core/stateexecutors/verifyingstateexecutor.h
    core/stateexecutors/verifyingstateexecutor.cpp
    core/stateexecutors/preparingstateexecutor.h
    core/stateexecutors/preparingstateexecutor.cpp
    core/stateexecutors/installingstateexecutor.h
    core/stateexecutors/installingstateexecutor.cpp
    core/deviceconfig.h
    core/deviceconfig.cpp
    core/updatecontext.h
    core/updatecontext.cpp
    core/stateexecutors/finalizingstateexecutor.h
    core/stateexecutors/finalizingstateexecutor.cpp
    core/stateexecutors/registrationexecutor.h
    core/stateexecutors/registrationexecutor.cpp
    core/stateexecutors/committingstateexecutor.h
    core/stateexecutors/committingstateexecutor.cpp
    core/interfaces/ihttpclient.h
    core/sslhttpclient.h
    core/interfaces/istatepersistance.h
    core/statepersistence.h
    core/statepersistence.cpp
    core/interfaces/iclientconfig.h

    core/interfaces/istateexecutor.h
    core/interfaces/isystemcalls.h
    core/systemcalls.h
)

target_compile_options(client-core PRIVATE --coverage -O0 -g)
target_link_libraries(client-core PRIVATE --coverage)
target_compile_definitions(client-core PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

add_executable(client-app main.cpp)

if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    find_library(ARCHIVE_LIB NAMES libarchive.a archive REQUIRED)
    message(STATIC!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!)
else()
    find_library(ARCHIVE_LIB NAMES archive REQUIRED)
    message(NOT STATIC!!!!!!!!!!!!!!!!)
endif()

target_link_libraries(client-app PUBLIC
    client-core
    common
    ${ARCHIVE_LIB}
    xml2
    icuuc
    icudata
    gmp
    nettle
    hogweed
    ssl
    crypto
    acl
    z
    bz2
    lzma
    zstd
    lz4
    dl
    pthread
    m
)

add_subdirectory(tests/client-test)

include(GNUInstallDirs)
install(TARGETS client-app
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
