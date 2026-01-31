cmake_minimum_required(VERSION 3.16)

project(utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    find_library(ARCHIVE_LIB NAMES libarchive.a archive REQUIRED)
else()
    find_library(ARCHIVE_LIB NAMES archive REQUIRED)
endif()

add_library(common STATIC
  sslutils.cpp
  sslutils.h
  archivetools.h
  archivetools.cpp
  task.h
  interfaces/iconfig.h
  interfaces/icryptoutils.h
  sslutilsadapter.h
  mocks/cryptoutils_mock.h
  mocks/archivetools_mock.h
  interfaces/iarchivetools.h
  archivetoolsadapter.h
)

target_link_libraries(common PRIVATE
  ${ARCHIVE_LIB}
  z
  bz2
  lzma
  zstd
  acl
  nettle
  xml2

  GTest::gtest_main
  GTest::gmock_main
)

target_compile_definitions(common PRIVATE UTILS_LIBRARY)

if(BUILD_STATIC)
    target_link_options(common PRIVATE -static)
endif()
