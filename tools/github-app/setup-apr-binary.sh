#!/bin/bash

# APR binary setup, simplified for pre-built executable
# the APT binary is now included in the repo

set -e

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
apr_binary_target="$script_dir/bin/apr_system"

echo "☞ initializing apr binary installation"
echo "––––––––––––––––––––––––––––––––––––––"

# ensure bin directory exists
mkdir -p "$script_dir/bin"

# confirm presence and permissions of the binary
if [[ -f "$apr_binary_target" ]]; then
    chmod +x "$apr_binary_target"
    
    echo "apr binary is prepared"
    echo "  location: $apr_binary_target"
    echo "  size: $(du -h "$apr_binary_target" | cut -f1)"
    echo ""
    echo "✓ installation complete"
else
    echo "✗ apr binary not detected at: $apr_binary_target"
    echo ""
    echo "please include the pre-built apr_system executable in the repository."
    echo "to add it manually, run:"
    echo "  cp ../../build/bin/apr_system ./bin/apr_system"
    echo "  chmod +x ./bin/apr_system"
    exit 1
fi
