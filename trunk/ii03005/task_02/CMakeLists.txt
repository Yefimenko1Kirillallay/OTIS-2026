cmake_minimum_required(VERSION 3.16)
project(Task02 LANGUAGES CXX)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

add_library(models src/models.cpp src/stability_analyzer.cpp)
target_include_directories(models PUBLIC src)
target_compile_options(models PRIVATE -Wall -Wextra -Wpedantic)
add_executable(Task02 src/main.cpp)
target_link_libraries(Task02 PRIVATE models)
target_compile_definitions(Task02 PRIVATE
    PYTHON_EXECUTABLE="${Python3_EXECUTABLE}"
    PLOT_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/src/plot.py"
    OUTPUT_DIRECTORY="${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable(Task02Tests test/models_test.cpp)
target_link_libraries(Task02Tests PRIVATE models GTest::gtest_main)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(models PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_options(models PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_options(Task02 PRIVATE -fprofile-arcs -ftest-coverage)
    target_compile_options(Task02Tests PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_options(Task02Tests PRIVATE -fprofile-arcs -ftest-coverage)
endif()
include(GoogleTest)
gtest_discover_tests(Task02Tests)
