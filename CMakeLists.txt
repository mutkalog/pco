cmake_minimum_required(VERSION 3.16)

project(posix-compatable-ota LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(HTTPLIB cpp-httplib REQUIRED)

add_subdirectory(client)
add_subdirectory(server)
