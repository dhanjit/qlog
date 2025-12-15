cmake_minimum_required(VERSION 3.14)
project(qlog VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Main library (Header only)
add_library(qlog INTERFACE)
target_include_directories(qlog INTERFACE include)

find_package(Threads REQUIRED)
target_link_libraries(qlog INTERFACE Threads::Threads)

# Dependencies
include(FetchContent)

# Google Test
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Google Benchmark
FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest googlebenchmark)

# Testing
enable_testing()

# Helper to add tests
function(add_qlog_test test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE qlog GTest::gtest_main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Helper to add benchmarks
function(add_qlog_benchmark bench_name source_file)
    add_executable(${bench_name} ${source_file})
    target_link_libraries(${bench_name} PRIVATE qlog benchmark::benchmark_main)
endfunction()

# Add subdirectories (we will populate these later)
# add_subdirectory(test)
# add_subdirectory(benchmark)

# Benchmark
add_qlog_benchmark(logger_benchmark test/benchmark/loggerbenchmark.cpp)

# Unit Tests
# Create a dummy test file if it doesn't exist yet, or we can add it later.
# For now, let's assume we will create test/unit_tests.cpp
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/unit_tests.cpp")
    add_qlog_test(unit_tests test/unit_tests.cpp)
endif()
