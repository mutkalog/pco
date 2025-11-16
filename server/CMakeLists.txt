add_executable(pco-server main.cpp)

# include_directories("/home/alexander/Projects/posix-compatable-ota/cpp-httplib")
# find_library(HTTPLIB cpp-httplib REQUIRED)
# target_link_libraries(pco-server PRIVATE ${HTTPLIB})

include(GNUInstallDirs)
install(TARGETS pco-server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
