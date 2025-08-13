cmake_minimum_required(VERSION 3.10)
project(buggy_sort)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")

add_library(sort_lib src/sort.cpp)
target_include_directories(sort_lib PUBLIC include)

add_executable(main_exec src/main.cpp)
target_link_libraries(main_exec sort_lib)

find_package(GTest REQUIRED)
add_executable(test_sort tests/test_sort.cpp)
target_link_libraries(test_sort sort_lib GTest::GTest GTest::Main)

enable_testing()
add_test(NAME SortTests COMMAND test_sort)

# Coverage target
add_custom_target(coverage
    COMMAND bash ${CMAKE_SOURCE_DIR}/get_coverage.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running coverage collection script"
)