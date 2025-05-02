#!/bin/sh
# Custom script to link the hurricane tests without including source files directly

# Clean up any previous builds
rm -f hurricane_tests

# Find all object files
OBJ_FILES=$(find ../build/tests -name "*.o" 2>/dev/null)

# Link only object files
echo "Linking with only object files..."
clang -Wall -Wextra -std=c11 -g $OBJ_FILES -o hurricane_tests

# Check if linking was successful
if [ $? -eq 0 ]; then
    echo "Link successful!"
    exit 0
else 
    echo "Link failed!"
    exit 1
fi