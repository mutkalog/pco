cmake_minimum_required(VERSION 3.16)

project(client_tests LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(client-unit-tests unit/main.cpp
  unit/stateexecutors/checkingstateexecutor_test.cpp)

target_link_libraries(client-unit-tests PRIVATE
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
  GTest::gtest_main
  GTest::gmock_main
)


include(GoogleTest)
gtest_discover_tests(client-unit-tests)
