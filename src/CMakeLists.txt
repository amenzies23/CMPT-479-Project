# create the main library
add_library(apr_system_lib
    core/types.h
    core/contracts.h
    core/logger.h
    core/logger.cpp

    cli/cli.h
    cli/cli.cpp

    sbfl/sbfl.h
    sbfl/sbfl.cpp

    parser/parser.h
    parser/parser.cpp

    mutator/mutator.h
    mutator/mutator.cpp
    mutator/context.h
    mutator/context.cpp 

    prioritizer/prioritizer.h
    prioritizer/prioritizer.cpp

    validator/validator.h
    validator/validator.cpp
    # validator/json_schema_validator.h
    # validator/json_schema_validator.cpp

    orchestrator/orchestrator.h
    orchestrator/orchestrator.cpp
)

# set include directories
target_include_directories(apr_system_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# link dependencies
target_link_libraries(apr_system_lib
    PUBLIC
        nlohmann_json::nlohmann_json
        fmt::fmt
        spdlog::spdlog
        # **ADDED THIS**
        unofficial::tree-sitter::tree-sitter
        tree-sitter-cpp-grammar
)

# create the main executable
add_executable(apr_system main.cpp)

# **ADDED THIS**
target_compile_definitions(apr_system_lib
    PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_compile_definitions(apr_system
    PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

# link the library
target_link_libraries(apr_system
    PRIVATE
    apr_system_lib
)

# set include directories for executable
target_include_directories(apr_system
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

