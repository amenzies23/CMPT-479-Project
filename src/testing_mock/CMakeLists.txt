cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

project(FaultLocalization
    VERSION 1.0
    DESCRIPTION "SBFL Example"
    LANGUAGES CXX)

# Set compiler flags for gcov
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-O0 -coverage")
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)

add_library(calculator_lib src/calculator.cpp)
target_include_directories(calculator_lib PUBLIC include)

add_executable(calculator src/main.cpp)
target_link_libraries(calculator calculator_lib)

enable_testing()
add_subdirectory(tests)
