project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/GroupControllerTest.cpp
        controller/MatchControllerTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/MatchDelegateTest.cpp
        repository/MatchRepositoryTest.cpp
        service/MatchGenerationServiceTest.cpp
        service/PlayoffGenerationServiceTest.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/controller/MatchController.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
)

set(SOURCES ${TEST_SOURCES})

find_package(GTest CONFIG REQUIRED)

include_directories(../include)

add_executable(${PROJECT_NAME}_runner
    ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        tournament_common
        libpqxx::pqxx
        -Wl,--start-group
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libpq.a
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libpgcommon.a
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libpgport.a
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libssl.a
        ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libcrypto.a
        -Wl,--end-group
        -ldl -lpthread)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)