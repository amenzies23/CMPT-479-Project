cmake_minimum_required(VERSION 3.10)
project(buggy_linked_list)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")

add_library(linked_list_lib src/linked_list.cpp)
target_include_directories(linked_list_lib PUBLIC include)

add_executable(main_exec main.cpp)
target_link_libraries(main_exec linked_list_lib)

find_package(GTest REQUIRED)
add_executable(test_linked_list tests/test_linked_list.cpp)
target_link_libraries(test_linked_list linked_list_lib GTest::GTest GTest::Main)

enable_testing()
add_test(NAME LinkedListTests COMMAND test_linked_list)

add_custom_target(coverage
	COMMAND bash ${CMAKE_SOURCE_DIR}/get_coverage.sh
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_dependencies(coverage test_linked_list)