#!/bin/bash

# Clean script for GDScript Compiler
# Removes all generated files and build artifacts

echo "Cleaning GDScript Compiler project..."

# Remove compiled binaries
find . -type f -name "*.o" -delete
find . -type f -name "*.s" -delete
find . -type f -name "*.exe" -delete
find . -type f -name "*.app" -delete
find . -type f -name "*_arm" -delete
find . -type f -name "*_linux" -delete
find . -type f -name "*_linux_*" -delete
find . -type f -name "*_windows" -delete
find . -type f -name "*_macos*" -delete

# Remove test files (except examples)
find . -type f -name "*.gd" -not -path "./examples/*" -delete

# Remove debug and utility binaries
find . -type f -name "dump_tokens" -delete
find . -type f -name "check_token_types" -delete
find . -type f -name "simple_dump_tokens" -delete

# Remove build directories
rm -rf obj/
rm -rf bin/
rm -rf *.dSYM/

# Remove test output
rm -rf test_output/

echo "Cleaning complete!"