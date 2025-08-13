add_custom_target(coverage
    COMMAND bash ${CMAKE_SOURCE_DIR}/get_coverage.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running coverage collection script"
)