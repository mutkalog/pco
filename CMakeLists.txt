cmake_minimum_required(VERSION 3.16)

project(posix-compatable-ota LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_library(HTTPLIB cpp-httplib REQUIRED)

add_executable(posix-compatable-ota main.cpp
    statemachine.h statemachine.cpp
    updater.h updater.cpp)

target_link_libraries(posix-compatable-ota PRIVATE ${HTTPLIB})

include(GNUInstallDirs)
install(TARGETS posix-compatable-ota
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
