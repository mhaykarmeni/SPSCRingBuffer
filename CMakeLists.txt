cmake_minimum_required(VERSION 3.20)
project(SPSCRingBuffer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(spsc_demo src/main.cpp)
target_include_directories(spsc_demo PRIVATE src)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(spsc_tests tests/spsc_queue_test.cpp)
target_include_directories(spsc_tests PRIVATE src)
target_link_libraries(spsc_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(spsc_tests)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(spsc_bench bench/spsc_queue_bench.cpp)
target_include_directories(spsc_bench PRIVATE src)
target_link_libraries(spsc_bench PRIVATE benchmark::benchmark)
