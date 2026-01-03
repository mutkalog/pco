cmake_minimum_required(VERSION 3.16)

project(server_tests LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server-tests main.cpp
  unit/database_test.cpp
  unit/connectionspool_test.cpp
  unit/service/rolloutsupervisor_test.cpp
  unit/service/updatesupervisor_test.cpp
  unit/service/uploadservice_test.cpp
  unit/service/servicefixturebase.h
  unit/service/manifestservice_test.cpp
  unit/service/downloadservice_test.cpp
  unit/service/reportservice_test.cpp
  unit/service/registrationservice_test.cpp
  unit/controllers/controllerfixturebase.h
  unit/controllers/uploadcontroller_test.cpp
  unit/service/mocks/uploadservice_mock.h
  unit/service/mocks/downloadservice_mock.h
  unit/controllers/downloadcontroller_test.cpp
  unit/controllers/manifestcontroller_test.cpp
  unit/service/mocks/manifestservice_mock.h
  unit/service/mocks/reportservice_mock.h
  unit/service/mocks/registrationservice_mock.h
  unit/controllers/registrationcontroller_test.cpp
  unit/controllers/reportcontroller_test.cpp
  integration/upload_IT.cpp
  integration/integrationbasefixture.h
  integration/download_IT.cpp
  system/systemfixturebase.h
  system/fullcycle_ST.cpp
  system/canaryrolloutsuccess_ST.cpp
  system/canaryrolloutfailure_ST.cpp
  system/canaryrollouttimeout_ST.cpp
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
