add_executable(pco-client main.cpp
    statemachine.h statemachine.cpp
    updater.h updater.cpp)

target_link_libraries(pco-client PRIVATE ${HTTPLIB})

include(GNUInstallDirs)
install(TARGETS pco-client
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
