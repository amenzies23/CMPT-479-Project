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

# **ADDED THIS**
# Define directory
add_compile_definitions(PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# enable testing
enable_testing()

# find required packages
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# **ADDED THIS**
add_subdirectory(tree-sitter)
add_subdirectory(tree-sitter-cpp)


# link other deps to apr_system_lib
target_link_libraries(apr_system_lib
    PUBLIC
        nlohmann_json::nlohmann_json
        fmt::fmt
        spdlog::spdlog
    # **ADDED THIS**
    PRIVATE
        tree-sitter
)

# **ADDED THIS**
target_include_directories(apr_system_lib PRIVATE ${CMAKE_SOURCE_DIR}/tree-sitter/lib/include)

# install configuration
include(GNUInstallDirs)
install(TARGETS apr_system apr_system_lib
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
