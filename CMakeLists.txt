cmake_minimum_required(VERSION 3.20)
project(hurricane_usb C)

set(CMAKE_C_STANDARD 11)

# Build options
option(ENABLE_USB_HOST "Enable USB Host stack" ON)
option(ENABLE_USB_DEVICE "Enable USB Device stack" ON)
option(ENABLE_DUAL_USB "Enable dual USB stack support" ON)
option(BUILD_EXAMPLES "Build example applications" ON)

# NXP SDK path configuration
set(NXP_SDK_PATH "/Users/ramseymcgrath/code/mcuxpresso-sdk/mcuxsdk" CACHE PATH "Path to NXP MCUXpresso SDK")
option(USE_PACKAGED_SDK "Use a packaged version of the NXP SDK" OFF)
set(NXP_SDK_PACKAGE_PATH "" CACHE PATH "Path to packaged NXP SDK (if not using direct SDK)")

# Platform configuration
if(NOT DEFINED PLATFORM)
    set(PLATFORM dummy)
endif()
message(STATUS "Using platform: ${PLATFORM}")
message(STATUS "USB Host support: ${ENABLE_USB_HOST}")
message(STATUS "USB Device support: ${ENABLE_USB_DEVICE}")
message(STATUS "Dual USB stack: ${ENABLE_DUAL_USB}")

# Set target-specific variables
if(PLATFORM STREQUAL "rt1060")
    set(TARGET "rt1060")
    set(HURRICANE_TARGET_DEVICE "MIMXRT1062" CACHE STRING "NXP MCU device name")
    message(STATUS "Building for RT1060 target")
    
    # Verify NXP SDK path is provided for RT1060 builds
    if(NOT NXP_SDK_PATH AND NOT USE_PACKAGED_SDK)
        message(FATAL_ERROR "NXP_SDK_PATH is required for RT1060 builds. Please specify -DNXP_SDK_PATH=<path to SDK>")
    elseif(USE_PACKAGED_SDK AND NOT NXP_SDK_PACKAGE_PATH)
        message(FATAL_ERROR "NXP_SDK_PACKAGE_PATH is required when USE_PACKAGED_SDK=ON. Please specify -DNXP_SDK_PACKAGE_PATH=<path to packaged SDK>")
    endif()
    
    # Include NXP SDK integration script for global configuration
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/nxp_sdk.cmake)
elseif(PLATFORM STREQUAL "lpc55s69")
    set(TARGET "lpc55s69")
    set(HURRICANE_TARGET_DEVICE "LPC55S69" CACHE STRING "NXP MCU device name")
    message(STATUS "Building for LPC55S69 target")
    
    # Verify NXP SDK path is provided for LPC55S69 builds
    if(NOT NXP_SDK_PATH AND NOT USE_PACKAGED_SDK)
        message(FATAL_ERROR "NXP_SDK_PATH is required for LPC55S69 builds. Please specify -DNXP_SDK_PATH=<path to SDK>")
    elseif(USE_PACKAGED_SDK AND NOT NXP_SDK_PACKAGE_PATH)
        message(FATAL_ERROR "NXP_SDK_PACKAGE_PATH is required when USE_PACKAGED_SDK=ON. Please specify -DNXP_SDK_PACKAGE_PATH=<path to packaged SDK>")
    endif()
    
    # Include NXP SDK integration script for global configuration
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/nxp_sdk.cmake)
elseif(PLATFORM STREQUAL "esp32")
    set(TARGET "esp32")
    message(STATUS "Building for ESP32 target")
endif()

# Core library
add_subdirectory(lib/hurricane)

# Platform-specific target
add_subdirectory(targets/${PLATFORM})

# Build examples if enabled and target is supported
if(BUILD_EXAMPLES)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/${TARGET})
        message(STATUS "Building examples for ${TARGET}")
        # Examples are added in the target's CMakeLists.txt
    else()
        message(STATUS "No examples available for ${TARGET}")
    endif()
endif()

# Print build configuration summary
message(STATUS "==== Hurricane USB Stack Build Configuration ====")
message(STATUS "Platform:         ${PLATFORM}")
message(STATUS "Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "USB Host:         ${ENABLE_USB_HOST}")
message(STATUS "USB Device:       ${ENABLE_USB_DEVICE}")
message(STATUS "Dual USB:         ${ENABLE_DUAL_USB}")
message(STATUS "Examples:         ${BUILD_EXAMPLES}")
message(STATUS "C Compiler:       ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "==============================================")
