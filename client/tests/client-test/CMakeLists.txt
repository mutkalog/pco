cmake_minimum_required(VERSION 3.16)

project(client_tests LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(client-tests main.cpp
  stateexecutors/checkingstateexecutor_test.cpp
  statepersistance_test.cpp
  deviceconfig_test.cpp
  artifactmanifest_test.cpp
  updatecontext_test.cpp
  mocks/deviceinfo_mock.h
  statemachine_test.cpp
  mocks/statepersistence_mock.h
  mocks/stateexecutor_mock.h
  mocks/httpclient_mock.h

  stateexecutors/downloadingstateexecutor_test.cpp
  stateexecutors/verifyingstateexecutor_test.cpp
  stateexecutors/executorsfixturebase.h
  mocks/syscalls_mock.h
  stateexecutors/preparingstateexecutor_mock.cpp
  stateexecutors/installingstateexecutor_test.cpp
  stateexecutors/committingstateexecutor_test.cpp
  stateexecutors/finalizingstateexecutor_test.cpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)

execute_process(
    COMMAND pkg-config --libs --static libxml-2.0
    OUTPUT_VARIABLE LIBXML2_STATIC_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_package(Threads REQUIRED)

target_link_libraries(client-tests PUBLIC
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

target_compile_options(client-tests PRIVATE --coverage -O0 -g)
target_link_libraries(client-tests PRIVATE --coverage)

include(GoogleTest)
gtest_discover_tests(client-tests)
