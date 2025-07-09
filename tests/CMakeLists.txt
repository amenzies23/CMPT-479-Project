# find gtest
find_package(GTest REQUIRED)

enable_testing()

# create test executable
add_executable(apr_tests
    unit/test_sbfl.cpp
    unit/test_parser.cpp
    unit/test_mutator.cpp
    unit/test_prioritizer.cpp
    unit/test_validator.cpp
    unit/test_prbot.cpp
    unit/test_orchestrator.cpp
    # test_json_schema_validator.cpp
    # integration/test_end_to_end.cpp
)

# set c++23 standard for consistency
set_property(TARGET apr_tests PROPERTY CXX_STANDARD 23)
set_property(TARGET apr_tests PROPERTY CXX_STANDARD_REQUIRED ON)

# link dependencies
target_link_libraries(apr_tests
    PRIVATE
        apr_system_lib
        GTest::gtest
        GTest::gtest_main
)

# set include directories
target_include_directories(apr_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# add tests
add_test(NAME apr_tests COMMAND apr_tests)

# set test properties
set_tests_properties(apr_tests PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "APR_TEST_MODE=1"
)
