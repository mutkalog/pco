cmake_minimum_required(VERSION 3.16)

project(utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mycrypto STATIC
  utils.cpp
  utils.h
)

target_compile_definitions(mycrypto PRIVATE UTILS_LIBRARY)
