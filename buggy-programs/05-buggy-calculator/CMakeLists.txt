cmake_minimum_required(VERSION 3.10)
project(buggy-calculator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")

# Add source files
add_library(calculator src/calculator.cpp)
target_include_directories(calculator PUBLIC include)

# Add tests
find_package(GTest REQUIRED)
add_executable(test_calculator tests/test_calculator.cpp)
target_link_libraries(test_calculator calculator GTest::GTest GTest::Main)

enable_testing()
add_test(NAME CalculatorTests COMMAND test_calculator) 
add_subdirectory(tests)