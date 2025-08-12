#!/bin/bash

set -e

PROJECT_ROOT="$(pwd)"

# build with coverage all buggy programs
for buggy_dir in "$PROJECT_ROOT"/*; do
    if [ -d "$buggy_dir" ]; then
        echo "Processing $buggy_dir"

        rm -rf "$buggy_dir/build/*"

        mkdir -p "$buggy_dir/build"
        cd "$buggy_dir/build"

        cmake ..
        make
        make coverage

        cd "$PROJECT_ROOT"

        echo "Finished $buggy_dir"
    fi
done