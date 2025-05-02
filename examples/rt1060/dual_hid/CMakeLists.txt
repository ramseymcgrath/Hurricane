# CMakeLists.txt for the Dual HID example using USB Interface Manager
# Demonstrates simultaneous USB host and device operation with the Interface Manager

cmake_minimum_required(VERSION 3.20)

# Set the project name
project(hurricane_dual_hid_example C)

# Target name and binary output
set(DUAL_HID_TARGET_NAME "hurricane_dual_hid")
set(DUAL_HID_BINARY "${CMAKE_CURRENT_BINARY_DIR}/${DUAL_HID_TARGET_NAME}.bin")

# Include NXP SDK components
include(${CMAKE_SOURCE_DIR}/cmake/nxp_sdk.cmake)

# Add the example sources
add_executable(${DUAL_HID_TARGET_NAME}
    main.c
    device_config.c
    host_handler.c
)

# Include directories with more comprehensive paths
target_include_directories(${DUAL_HID_TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/hurricane
    ${CMAKE_SOURCE_DIR}/lib/hurricane/core
    ${CMAKE_SOURCE_DIR}/lib/hurricane/usb
    ${CMAKE_SOURCE_DIR}/lib/hurricane/hw
)

# Link against Hurricane libraries
target_link_libraries(${DUAL_HID_TARGET_NAME}
    hurricane
    hurricane_rt1060_hal
)

# Add NXP SDK components needed for this example
add_nxp_sdk_components(${DUAL_HID_TARGET_NAME}
    USB_HOST
    USB_DEVICE
    USB_HID_HOST
    USB_HID_DEVICE
    GPIO
    CLOCK
)

# Generate NXP SDK configuration headers specifically for this example
generate_nxp_sdk_config(${DUAL_HID_TARGET_NAME})

# Set compiler flags
target_compile_options(${DUAL_HID_TARGET_NAME} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-d16
    -Wall
    -Wextra
    -Werror
    -ffunction-sections
    -fdata-sections
    -fno-common
    -O2
    -Wno-unused-parameter
)

# Set compiler definitions
target_compile_definitions(${DUAL_HID_TARGET_NAME} PRIVATE
    CPU_MIMXRT1062DVL6A
    HURRICANE_RT1060
    DUAL_USB_SUPPORT
    HURRICANE_DUAL_HID_EXAMPLE
)

# Linker script and options
target_link_options(${DUAL_HID_TARGET_NAME} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-d16
    --specs=nano.specs
    --specs=nosys.specs
    -Wall
    -Wl,--gc-sections
    -fno-common
    -ffunction-sections
    -fdata-sections
    "-T${CMAKE_SOURCE_DIR}/lib/hurricane/hw/boards/rt1060/linker.ld"
    -Wl,--print-memory-usage
)

# Binary file generation
add_custom_command(TARGET ${DUAL_HID_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_CURRENT_BINARY_DIR}/${DUAL_HID_TARGET_NAME} ${DUAL_HID_BINARY}
    COMMENT "Generating binary file ${DUAL_HID_BINARY}"
)

# Create flash target for easy programming
add_custom_target(flash_dual_hid
    COMMAND ${CMAKE_SOURCE_DIR}/tools/flash.sh ${DUAL_HID_BINARY}
    DEPENDS ${DUAL_HID_TARGET_NAME}
    COMMENT "Flashing dual HID example to RT1060 board"
)

# Installation for the example binary
install(TARGETS ${DUAL_HID_TARGET_NAME}
    RUNTIME DESTINATION bin/examples
)

# Print success message
message(STATUS "Configured dual HID example with USB Interface Manager")