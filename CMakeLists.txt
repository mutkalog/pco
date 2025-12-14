cmake_minimum_required(VERSION 3.16)

project(posix-compatable-ota LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("../cpp-httplib")
include_directories("utils")
include_directories("server")
include_directories("client")

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/065127f1e4b46c5f14fc73cf8d323c221f9dc68e.zip
)
FetchContent_MakeAvailable(googletest)

add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(utils)
