cmake_minimum_required(VERSION 3.16)

project(client_tests LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(client-tests main.cpp
  unit/stateexecutors/checkingstateexecutor_test.cpp
  unit/statepersistance_test.cpp
  unit/deviceconfig_test.cpp
  unit/artifactmanifest_test.cpp
  unit/updatecontext_test.cpp
  unit/mocks/deviceinfo_mock.h
  unit/statemachine_test.cpp
  unit/mocks/statepersistence_mock.h
  unit/mocks/stateexecutor_mock.h
  unit/mocks/httpclient_mock.h
  unit/stateexecutors/downloadingstateexecutor_test.cpp
  unit/stateexecutors/verifyingstateexecutor_test.cpp
  unit/stateexecutors/executorsfixturebase.h
  unit/mocks/syscalls_mock.h
  unit/stateexecutors/preparingstateexecutor_mock.cpp
  unit/stateexecutors/installingstateexecutor_test.cpp
  unit/stateexecutors/committingstateexecutor_test.cpp
  unit/stateexecutors/finalizingstateexecutor_test.cpp
  integration/integrationfixturebase.h
  integration/registrationstateexecutor_IT.cpp
  integration/checkingstateexecutor_IT.cpp
  integration/downloadingstateexecute_IT.cpp
  integration/verifyingstateexecutor_IT.cpp
  integration/preparingstateexecutor_IT.cpp
  integration/installingstateexecutor.cpp
  integration/finalizingstateexecutor_IT.cpp
  integration/committingstateexecutor_IT.cpp
  integration/restartduringupdateinverifyingstate_IT.cpp
  integration/restartduringprocessinpreparingstate_IT.cpp
  integration/restartduringprocessininstallingstate_IT.cpp
  integration/restartduringprocessincommittingstate_IT.cpp
  integration/restarduringprocessInfinalizingstate_IT.cpp
  integration/updatewithexistinglastupdatefile_IT.cpp
  system/fullcycleupdate_ST.cpp
  system/systemfixturebase.h
  system/chainedupdate_ST.cpp
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

# target_compile_options(client-tests PRIVATE --coverage -O0 -g)
# target_link_libraries(client-tests PRIVATE --coverage)

include(GoogleTest)
gtest_discover_tests(client-tests)
