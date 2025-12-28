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

if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    find_library(ARCHIVE_LIB NAMES libarchive.a archive REQUIRED)
else()
    find_library(ARCHIVE_LIB NAMES archive REQUIRED)
endif()

target_link_libraries(client-tests PRIVATE
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

  GTest::gtest_main
  GTest::gmock_main
)

target_compile_options(client-tests PRIVATE --coverage -O0 -g)
target_link_libraries(client-tests PRIVATE --coverage)

include(GoogleTest)
gtest_discover_tests(client-tests)
