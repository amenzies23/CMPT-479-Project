#!/bin/bash

set -e

PROJECT_ROOT="$(pwd)"

# remove all build directories in buggy programs
for buggy_dir in "$PROJECT_ROOT"/*; do
    if [ -d "$buggy_dir/build" ]; then
        rm -rf "$buggy_dir/build"
    fi
done

# build with coverage all buggy programs
for buggy_dir in "$PROJECT_ROOT"/*; do
    if [ -d "$buggy_dir" ]; then
        echo "Processing $buggy_dir"

        mkdir -p "$buggy_dir/build"
        cd "$buggy_dir/build"

        cmake ..
        make
        make coverage

        echo "Finished $buggy_dir"
    fi
done