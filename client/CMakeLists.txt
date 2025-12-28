set(CMAKE_EXE_LINKER_FLAGS "-static")
set(ARCHIVE_LIB /usr/lib/x86_64-linux-gnu/libarchive.a)

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
    core/interfaces/iartifactmanifest.h
    core/interfaces/istateexecutor.h
    core/interfaces/isystemcalls.h
    core/systemcalls.h
)

target_compile_options(client-core PRIVATE --coverage -O0 -g)
target_link_libraries(client-core PRIVATE --coverage)
target_compile_definitions(client-core PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

add_executable(client-app main.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)

execute_process(
    COMMAND pkg-config --libs --static libxml-2.0
    OUTPUT_VARIABLE LIBXML2_STATIC_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_package(Threads REQUIRED)

target_link_libraries(client-app PUBLIC
    client-core
    common
    gmp
    nettle
    hogweed
    bz2
    zstd
    lz4
    ssl
    crypto
    acl
    ${LIBXML2_STATIC_LIBS}
    z
    lzma
    bz2
    Threads::Threads
    dl
    m
    GTest::gtest_main
    GTest::gmock_main
)

add_subdirectory(tests/client-test)

include(GNUInstallDirs)
install(TARGETS client-app
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
