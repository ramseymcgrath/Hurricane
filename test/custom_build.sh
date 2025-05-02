#!/bin/bash
# Custom build script for Hurricane tests

set -e  # Exit on any error

echo "=== Custom Build for Hurricane Tests ==="

# Clean previous builds
echo "Cleaning..."
rm -rf ../build/tests
mkdir -p ../build/tests
rm -f hurricane_tests

# Directories
BUILD_DIR="../build/tests"
CORE_DIR="../lib/hurricane/core"
USB_DIR="../lib/hurricane/usb"
DUMMY_HAL_DIR="../lib/hurricane/hw/boards/dummy"

# Find all source files
CORE_SRC=$(find $CORE_DIR -name "*.c")
USB_SRC=$(find $USB_DIR -name "*.c")
HAL_SRC=$(find $DUMMY_HAL_DIR -name "*.c")
# Make sure our new fix file is included
echo "Including HAL fix file: $DUMMY_HAL_DIR/usb_hw_hal_dummy_fix.c"
TEST_SRC="test_runner.c $(find unit -name "*.c")"

# Create output directories
mkdir -p $BUILD_DIR/core
mkdir -p $BUILD_DIR/usb
mkdir -p $BUILD_DIR/hal
mkdir -p $BUILD_DIR/test
mkdir -p $BUILD_DIR/unit

# Include paths
INCLUDES="-I../lib/hurricane -I../lib/hurricane/core -I../lib/hurricane/usb -I../lib/hurricane/hw -I../lib/hurricane/hw/boards/dummy -I.."

# Compile core files
echo "Compiling core files..."
for src in $CORE_SRC; do
  obj="$BUILD_DIR/$(basename ${src%.c}.o)"
  echo "  $src -> $obj"
  clang $INCLUDES -Wall -Wextra -std=c11 -g -c "$src" -o "$obj"
done

# Compile USB files
echo "Compiling USB files..."
for src in $USB_SRC; do
  obj="$BUILD_DIR/$(basename ${src%.c}.o)"
  echo "  $src -> $obj"
  clang $INCLUDES -Wall -Wextra -std=c11 -g -c "$src" -o "$obj"
done

# Compile HAL files
echo "Compiling HAL files..."
for src in $HAL_SRC; do
  obj="$BUILD_DIR/$(basename ${src%.c}.o)"
  echo "  $src -> $obj"
  clang $INCLUDES -Wall -Wextra -std=c11 -g -c "$src" -o "$obj"
done

# Compile test files
echo "Compiling test files..."
for src in $TEST_SRC; do
  base=$(basename $src)
  obj="$BUILD_DIR/${base%.c}.o"
  echo "  $src -> $obj"
  clang $INCLUDES -Wall -Wextra -std=c11 -g -c "$src" -o "$obj"
done

# Link everything
echo "Linking hurricane_tests..."
OBJ_FILES=$(find $BUILD_DIR -name "*.o")
clang -Wall -Wextra -std=c11 -g $OBJ_FILES -o hurricane_tests

echo "Build complete! Run with: ./hurricane_tests"