cmake_minimum_required(VERSION 3.16)
project(Lab5NamedPipes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(lab5_core
    src/EmployeeRepository.cpp
    src/RecordLockManager.cpp
)
target_include_directories(lab5_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

add_executable(Server src/Server.cpp)
target_link_libraries(Server PRIVATE lab5_core)

add_executable(Client src/Client.cpp)
target_link_libraries(Client PRIVATE lab5_core)

include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(Lab5Tests tests/Lab5Tests.cpp)
    target_link_libraries(Lab5Tests PRIVATE lab5_core GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(Lab5Tests)
endif()
