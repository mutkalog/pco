cmake_minimum_required(VERSION 3.16)

project(utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_library(ARCHIVE_LIB archive)

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(ARCHIVE_LIB /usr/lib/x86_64-linux-gnu/libarchive.a)
# include_directories("/home/alexander/Projects/posix-compatable-ota/cpp-httplib")


add_library(common STATIC
  sslutils.cpp
  sslutils.h
  archivetools.h archivetools.cpp

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
  lz4
  ssl
  crypto
  acl
  gmp
  nettle
  hogweed
  xml2
  GTest::gtest_main
  GTest::gmock_main
)

target_link_options(common PRIVATE -static)

target_compile_definitions(common PRIVATE UTILS_LIBRARY)
