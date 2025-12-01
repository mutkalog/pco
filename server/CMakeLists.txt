add_executable(pco-server main.cpp
    http/uploadcontroller.h http/uploadcontroller.cpp
    http/server.h http/server.cpp
    http/controller.h http/controller.cpp
    http/manifestcontrller.h http/manifestcontrller.cpp
    http/downloadcontroller.h http/downloadcontroller.cpp
    http/reportcontroller.h http/reportcontroller.cpp
    http/registrationcontroller.h http/registrationcontroller.cpp
    service/uploadservice.h service/uploadservice.cpp
    service/manifestservice.h service/manifestservice.cpp)

# include_directories("/home/alexander/Projects/posix-compatable-ota/cpp-httplib")
# find_library(HTTPLIB cpp-httplib REQUIRED)
# target_link_libraries(pco-server PRIVATE ${HTTPLIB})
target_compile_definitions(pco-server PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

target_link_libraries(pco-server PRIVATE
    common
    serverside
    ssl
    crypto
)

include(GNUInstallDirs)
install(TARGETS pco-server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
