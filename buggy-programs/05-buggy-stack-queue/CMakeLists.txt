cmake_minimum_required(VERSION 3.10)
project(buggy_stack_queue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")

add_library(stack_queue_lib src/stack-queue.cpp)
target_include_directories(stack_queue_lib PUBLIC include)

find_package(GTest REQUIRED)
add_executable(test_stack_queue tests/test_stack_queue.cpp)
target_link_libraries(test_stack_queue stack_queue_lib GTest::GTest GTest::Main)

enable_testing()
add_test(NAME StackQueueTests COMMAND test_stack_queue)

# Coverage target
add_custom_target(coverage
    COMMAND bash ${CMAKE_SOURCE_DIR}/get_coverage.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running coverage collection script"
)