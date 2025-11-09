project(tournament_services)

set(CMAKE_CXX_STANDARD 23)
set(SERVICES_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/delegate/TeamDelegate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/delegate/TournamentDelegate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/controller/TournamentController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/controller/TeamController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/controller/MatchController.cpp)

include(CTest)
enable_testing()

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)

# Find ActiveMQ using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(ACTIVEMQ REQUIRED activemq-cpp)


add_subdirectory(tests)

include_directories(include)

add_executable(${PROJECT_NAME}
        main.cpp
        ${SERVICES_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        tournament_common
        -Wl,--start-group
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libpq.a
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libpgcommon.a
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libpgport.a
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libssl.a
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libcrypto.a
        -Wl,--end-group
        -ldl -lpthread)

# Link ActiveMQ using pkg-config
target_link_directories(${PROJECT_NAME} PRIVATE ${ACTIVEMQ_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${ACTIVEMQ_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${ACTIVEMQ_INCLUDE_DIRS})
target_compile_options(${PROJECT_NAME} PRIVATE ${ACTIVEMQ_CFLAGS_OTHER})

target_include_directories(${PROJECT_NAME} INTERFACE ${HYPODERMIC_INCLUDE_DIRS})

# Copy configuration file if it exists (optional for tests)
if(EXISTS ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json)
    configure_file(
            ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json   # source file
            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json  # destination
            COPYONLY)
endif()