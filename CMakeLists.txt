cmake_minimum_required(VERSION 3.16)

project(posix-compatable-ota LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.28.0
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/065127f1e4b46c5f14fc73cf8d323c221f9dc68e.zip
)
FetchContent_MakeAvailable(googletest)

set(BUILD_STATIC OFF CACHE BOOL "Build statically linked binaries" FORCE)

include_directories(${httplib_SOURCE_DIR})

include_directories("common")
include_directories("server")
include_directories("client")

add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(common)
