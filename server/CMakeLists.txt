add_library(server-core
    core/http/uploadcontroller.h
    core/http/uploadcontroller.cpp
    core/http/server.h
    core/http/controller.h
    core/http/manifestcontrller.h
    core/http/manifestcontrller.cpp
    core/http/downloadcontroller.h
    core/http/downloadcontroller.cpp
    core/http/reportcontroller.h
    core/http/reportcontroller.cpp
    core/http/registrationcontroller.h
    core/http/registrationcontroller.cpp
    core/service/uploadservice.h
    core/service/uploadservice.cpp
    core/service/manifestservice.h
    core/service/manifestservice.cpp
    core/service/downloadservice.h
    core/service/downloadservice.cpp
    core/service/registrationservice.h
    core/service/registrationservice.cpp
    core/service/reportservice.h
    core/service/reportservice.cpp
    core/servercontext.h
    core/updatesupervisor.h
    core/updatesupervisor.cpp
    core/rolloutsupervisor.h
    core/rolloutsupervisor.cpp
    core/database.h
    core/database.cpp
    core/connectionspull.h
    core/service/interfaces/iuploadservice.h
    core/service/interfaces/idownloadservice.h
    core/service/interfaces/iregistrationservice.h
    core/service/interfaces/ireportservice.h
    core/service/interfaces/imanifestservice.h
)

target_compile_options(server-core PRIVATE --coverage -O0 -g)
target_link_libraries(server-core PRIVATE --coverage)
target_compile_definitions(server-core PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

add_executable(server-app main.cpp)

target_link_libraries(server-app PUBLIC
    server-core
    common
    ssl
    crypto
    pqxx
)

add_subdirectory(tests/server-test)


include(GNUInstallDirs)
install(TARGETS server-app
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
