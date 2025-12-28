cmake_minimum_required(VERSION 3.16)

project(server_tests LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server-tests main.cpp
  database_test.cpp
  connectionspool_test.cpp
  service/rolloutsupervisor_test.cpp
  service/updatesupervisor_test.cpp
  service/uploadservice_test.cpp
  service/servicefixturebase.h
  service/manifestservice_test.cpp
  service/downloadservice_test.cpp
  service/reportservice_test.cpp
  service/registrationservice_test.cpp
  controllers/controllerfixturebase.h
  controllers/uploadcontroller_test.cpp
  service/mocks/uploadservice_mock.h
  service/mocks/downloadservice_mock.h
  controllers/downloadcontroller_test.cpp
  controllers/manifestcontroller_test.cpp
  service/mocks/manifestservice_mock.h
  service/mocks/reportservice_mock.h
  service/mocks/registrationservice_mock.h
  controllers/registrationcontroller_test.cpp
  controllers/reportcontroller_test.cpp

)

target_link_libraries(server-tests PRIVATE
  server-core
  common
  pqxx
  pq
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
gtest_discover_tests(server-tests)
