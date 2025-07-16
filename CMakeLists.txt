cmake_minimum_required(VERSION 3.20)
project(apr_system VERSION 1.0.0 LANGUAGES CXX C)

# prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR 
        "In-source builds are not allowed. Please create a separate build directory:\n"
        "  mkdir build\n"
        "  cd build\n"
        "  cmake ..\n"
        "This protects your source tree from build artifacts.")
endif()

# configure out-of-source build directory structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# create logs directory in build folder
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/results)

# set c++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic -Wno-unused-parameter -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra -Wno-unused-parameter")

add_compile_definitions(PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# enable testing
enable_testing()

# --- vcpkg integration for dependencies ---
find_package(unofficial-tree-sitter CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
# --- end vcpkg integration ---

# --- tree-sitter-cpp grammar integration ---
if(EXISTS ${CMAKE_SOURCE_DIR}/tree-sitter-cpp-grammar/src/parser.c)
    set(TREE_SITTER_CPP_GRAMMAR_SOURCES
        ${CMAKE_SOURCE_DIR}/tree-sitter-cpp-grammar/src/parser.c
        ${CMAKE_SOURCE_DIR}/tree-sitter-cpp-grammar/src/scanner.cc
    )
    set(TREE_SITTER_CPP_GRAMMAR_INCLUDE ${CMAKE_SOURCE_DIR}/tree-sitter-cpp-grammar/src)

# found docker tree-sitter-cpp grammar
elseif(EXISTS /opt/tree-sitter-cpp-grammar/src/parser.c)
    set(TREE_SITTER_CPP_GRAMMAR_SOURCES
        /opt/tree-sitter-cpp-grammar/src/parser.c
        /opt/tree-sitter-cpp-grammar/src/scanner.cc
    )
    set(TREE_SITTER_CPP_GRAMMAR_INCLUDE /opt/tree-sitter-cpp-grammar/src)
else()
    message(FATAL_ERROR "tree-sitter-cpp grammar not found. Please run: git clone --depth 1 --branch v0.20.0 https://github.com/tree-sitter/tree-sitter-cpp.git tree-sitter-cpp-grammar")
endif()

add_library(tree-sitter-cpp-grammar STATIC ${TREE_SITTER_CPP_GRAMMAR_SOURCES})
target_include_directories(tree-sitter-cpp-grammar PUBLIC $<BUILD_INTERFACE:${TREE_SITTER_CPP_GRAMMAR_INCLUDE}>)
target_link_libraries(tree-sitter-cpp-grammar PUBLIC unofficial::tree-sitter::tree-sitter)
# --- end tree-sitter-cpp grammar integration ---

# add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# link other deps to apr_system_lib
target_link_libraries(apr_system_lib
    PUBLIC
        nlohmann_json::nlohmann_json
        fmt::fmt
        spdlog::spdlog
        unofficial::tree-sitter::tree-sitter
        tree-sitter-cpp-grammar
)

# install configuration
include(GNUInstallDirs)
install(TARGETS apr_system apr_system_lib tree-sitter-cpp-grammar
    EXPORT apr_systemTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# install headers
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/apr_system
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    PATTERN "*/private/*" EXCLUDE
)

# export targets
install(EXPORT apr_systemTargets
    FILE apr_systemTargets.cmake
    NAMESPACE apr_system::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/apr_system
)
