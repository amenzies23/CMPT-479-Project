cmake_minimum_required(VERSION 3.10)
project(buggy_visitor_pattern)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")

# Add source files
set(SOURCES
	src/rectangle.cpp
	src/circle.cpp
	src/triangle.cpp
	src/area_calculator.cpp
)

add_library(area_lib ${SOURCES})
target_include_directories(area_lib PUBLIC include)

add_executable(main_exec main.cpp)
target_link_libraries(main_exec area_lib)

find_package(GTest REQUIRED)
add_executable(test_area_calculator tests/test_area_calculator.cpp)
target_link_libraries(test_area_calculator area_lib GTest::GTest GTest::Main)

enable_testing()
add_test(NAME AreaCalculatorTests COMMAND test_area_calculator)

add_custom_target(coverage
	COMMAND bash ${CMAKE_SOURCE_DIR}/get_coverage.sh
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_dependencies(coverage test_area_calculator)