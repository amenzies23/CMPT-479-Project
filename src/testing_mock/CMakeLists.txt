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

add_library(add_lib src/add.cpp)
target_include_directories(add_lib PUBLIC include)

add_executable(add src/main.cpp)
target_link_libraries(add add_lib)

enable_testing()
add_subdirectory(tests)
