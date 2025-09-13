cmake_minimum_required(VERSION 3.31)
project(parser)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(parser main.cpp Parser.h Parser.cpp)
# Link against Threads for std::thread support
find_package(Threads REQUIRED)
target_link_libraries(parser PRIVATE Threads::Threads)


enable_testing()
include(GoogleTest)

add_executable(runTests tests.cpp Parser.cpp)

target_link_libraries(runTests PRIVATE GTest::gtest_main Threads::Threads)

gtest_discover_tests(runTests)
