cmake_minimum_required(VERSION 3.16)
project(Lab4SyncProcesses LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(queue_file_lib src/QueueFile.cpp)
target_include_directories(queue_file_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

add_executable(Receiver src/Receiver.cpp)
target_link_libraries(Receiver PRIVATE queue_file_lib)

add_executable(Sender src/Sender.cpp)
target_link_libraries(Sender PRIVATE queue_file_lib)

include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(QueueFileTests tests/QueueFileTests.cpp)
    target_link_libraries(QueueFileTests PRIVATE queue_file_lib GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(QueueFileTests)
endif()
