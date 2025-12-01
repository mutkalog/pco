cmake_minimum_required(VERSION 3.16)

project(serverside LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(serverside STATIC
  database.h database.cpp
)

target_link_libraries(serverside PRIVATE  pqxx
)

target_compile_definitions(serverside PRIVATE SERVERSIDE_LIBRARY)
