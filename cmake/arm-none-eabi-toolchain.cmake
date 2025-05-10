# ARM cross-compiler toolchain file for Hurricane project
# This file configures CMake to use the ARM GCC toolchain for cross-compilation

# Specify the target system
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify ARM toolchain paths
# First check if we have the ARM GCC installed via Homebrew (which is the case for this system)
if(EXISTS "/opt/homebrew/bin/arm-none-eabi-gcc")
    set(ARM_TOOLCHAIN_DIR "/opt/homebrew")
    message(STATUS "Using ARM GNU Toolchain from Homebrew")
# Check for ARM's official package as a fallback
elseif(EXISTS "/Applications/ArmGNUToolchain/14.2.rel1/arm-none-eabi")
    set(ARM_TOOLCHAIN_DIR "/Applications/ArmGNUToolchain/14.2.rel1")
    message(STATUS "Using ARM GNU Toolchain from official package")
else()
    message(FATAL_ERROR "ARM GCC toolchain not found. Please install it using Homebrew or from the official ARM website.")
endif()

# Programs for cross-compiling
set(CMAKE_C_COMPILER "${ARM_TOOLCHAIN_DIR}/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${ARM_TOOLCHAIN_DIR}/bin/arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_DIR}/bin/arm-none-eabi-gcc")
set(CMAKE_OBJCOPY "${ARM_TOOLCHAIN_DIR}/bin/arm-none-eabi-objcopy")
set(CMAKE_OBJDUMP "${ARM_TOOLCHAIN_DIR}/bin/arm-none-eabi-objdump")
set(CMAKE_SIZE "${ARM_TOOLCHAIN_DIR}/bin/arm-none-eabi-size")
set(CMAKE_DEBUGGER "${ARM_TOOLCHAIN_DIR}/bin/arm-none-eabi-gdb")
set(CMAKE_CPPFILT "${ARM_TOOLCHAIN_DIR}/bin/arm-none-eabi-c++filt")

# Skip compiler test during configure
# This prevents CMake from trying to build and link a test program
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# When this toolchain file is used, don't try to search for programs on the host system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Common compiler flags for ARM Cortex-M33
set(COMMON_FLAGS "-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-exceptions")

# Set C compiler flags for ARM
set(CMAKE_C_FLAGS "${COMMON_FLAGS} -Wall -Wextra -Warray-bounds -std=gnu11" CACHE STRING "C compiler flags")
# Set C++ compiler flags
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -Wall -Wextra -Warray-bounds -std=gnu++17 -fno-rtti -fno-exceptions" CACHE STRING "C++ compiler flags")

# Set linker flags for bare-metal environment
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -nostartfiles -nodefaultlibs -nostdlib -specs=nano.specs -specs=nosys.specs" CACHE STRING "Linker flags")

# Debug flags
set(CMAKE_C_FLAGS_DEBUG "-Og -g -DDEBUG" CACHE STRING "C compiler flags for Debug build type")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g -DDEBUG" CACHE STRING "C++ compiler flags for Debug build type")

# Release flags
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "C compiler flags for Release build type")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "C++ compiler flags for Release build type")

# Set the default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose build type: Debug Release" FORCE)
endif()

# Set additional search paths for the toolchain
set(CMAKE_FIND_ROOT_PATH
    ${ARM_TOOLCHAIN_DIR}/arm-none-eabi
)

message(STATUS "Cross-compilation toolchain configured for ARM")