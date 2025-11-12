add_executable(pco-server main.cpp)

target_link_libraries(pco-server PRIVATE ${HTTPLIB})

include(GNUInstallDirs)
install(TARGETS pco-server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
