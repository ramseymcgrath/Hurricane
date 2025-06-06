# LPC55S69 target with dual USB stack support
cmake_minimum_required(VERSION 3.20)

# Set target device and include NXP SDK integration script
set(HURRICANE_TARGET_DEVICE "LPC55S69" CACHE STRING "NXP MCU device name")

# Source files for the LPC55S69 target
file(GLOB MAIN_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

# LPC55S69 target specific definitions
set(LPC55S69_TARGET_NAME "hurricane_lpc55s69")
set(LPC55S69_FLASH_BINARY "${CMAKE_CURRENT_BINARY_DIR}/${LPC55S69_TARGET_NAME}.bin")

# Create LPC55S69 executable with main sources - only define it once
add_executable(${LPC55S69_TARGET_NAME}
    ${MAIN_SRC}
)

# Now include the NXP SDK after the target is created
include(${CMAKE_SOURCE_DIR}/cmake/nxp_sdk.cmake)

# Create the HAL library for LPC55S69
add_library(hurricane_lpc55s69_hal
    # HAL implementation files for LPC55S69
    ${CMAKE_SOURCE_DIR}/lib/hurricane/hw/boards/lpc5500/usb_hw_init_lpc55s69.c
    
    # Include main.c as a backup source file
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

# Add include directories for the HAL library
target_include_directories(hurricane_lpc55s69_hal PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/hurricane
    ${CMAKE_SOURCE_DIR}/lib/hurricane/core
    ${CMAKE_SOURCE_DIR}/lib/hurricane/hw
    ${CMAKE_SOURCE_DIR}/lib/hurricane/usb
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69/device
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69/CMSIS
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69/usb/include
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69/component/osa
    ${CMAKE_CURRENT_BINARY_DIR}/nxp_sdk_config
    # Add NXP SDK paths for the LPC55S69 HAL library
    ${NXP_SDK_PATH}/devices/LPC/LPC5500/LPC55S69
    ${NXP_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers
    ${NXP_SDK_PATH}/devices/LPC/LPC5500/periph
    ${NXP_SDK_PATH}/devices/LPC/LPC5500/periph1
    ${NXP_SDK_PATH}/devices/LPC/LPC5500/periph2
    ${NXP_SDK_PATH}/devices/LPC/LPC5500/periph3
    ${NXP_SDK_PATH}/CMSIS/Core/Include
    ${NXP_SDK_PATH}/arch/arm/CMSIS/Core/Include
    ${NXP_SDK_PATH}/drivers/common
    # Add debug console component
    ${NXP_SDK_PATH}/components/debug_console
    ${NXP_SDK_PATH}/components/serial_manager
    # Add OS abstraction component
    ${NXP_SDK_PATH}/components/osa
    ${NXP_SDK_PATH}/components/osa/config
)

# Add compiler options for HAL library to include our USB host configuration fixes
target_compile_options(hurricane_lpc55s69_hal PRIVATE
    -include "${CMAKE_SOURCE_DIR}/lib/hurricane/core/usb_host_config_fix.h"
)

# Also add SDK components to the HAL library
add_nxp_sdk_components(hurricane_lpc55s69_hal
    USB_HOST
    USB_DEVICE
    USB_HID_HOST
    USB_HID_DEVICE
    platform.drivers.gpio # Changed from driver.gpio
    CLOCK
)

# Add CPU definitions for the HAL library
target_compile_definitions(hurricane_lpc55s69_hal PUBLIC
    CPU_LPC55S69JBD100_cm33_core0
    # __CORTEX_M=33  # Removed to avoid conflict with core_cm33.h
    __FPU_PRESENT=1
    __NEWLIB__
    __USE_CMSIS
)

# Include directories
target_include_directories(${LPC55S69_TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/hurricane
    ${CMAKE_SOURCE_DIR}/lib/hurricane/core
    ${CMAKE_SOURCE_DIR}/lib/hurricane/hw
    ${CMAKE_SOURCE_DIR}/lib/hurricane/usb
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69/device
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69/CMSIS
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69/usb/include
    ${CMAKE_SOURCE_DIR}/mcu_workspace/Hurricane_LPC55S69/component/osa
    ${CMAKE_CURRENT_BINARY_DIR}/nxp_sdk_config
)

# Link against our LPC55S69 HAL library and Hurricane core
target_link_libraries(${LPC55S69_TARGET_NAME}
    hurricane_lpc55s69_hal
    hurricane
)

# Add NXP SDK components with more comprehensive list
# Now using our updated nxp_sdk.cmake script with LPC55S69 support
add_nxp_sdk_components(${LPC55S69_TARGET_NAME}
    USB_HOST
    USB_DEVICE
    USB_HID_HOST
    USB_HID_DEVICE
    platform.drivers.gpio # Changed from driver.gpio
    CLOCK
)

# Generate NXP SDK configuration headers
generate_nxp_sdk_config(${LPC55S69_TARGET_NAME})

# LPC55S69-specific compiler options
target_compile_options(${LPC55S69_TARGET_NAME} PRIVATE
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-sp-d16
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
    -fno-common
    -fmessage-length=0
    -Wno-unused-parameter
    -Wno-unused-variable
    -O2
    -include "${CMAKE_SOURCE_DIR}/lib/hurricane/core/usb_host_config_fix.h"
)

# LPC55S69-specific compiler definitions
target_compile_definitions(${LPC55S69_TARGET_NAME} PRIVATE
    CPU_LPC55S69JBD100
    USB_STACK_BM
    HURRICANE_LPC55S69
    DUAL_USB_SUPPORT
    __NEWLIB__
    __USE_CMSIS
    NDEBUG
)

# LPC55S69-specific linker options
target_link_options(${LPC55S69_TARGET_NAME} PRIVATE
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-sp-d16
    -nostdlib
    -Wall
    -Wl,--gc-sections
    -fno-common
    -ffunction-sections
    -fdata-sections
    -Wl,--print-memory-usage
    -T${LPC55S69_LINKER_SCRIPT}
)

# Binary file generation
add_custom_command(TARGET ${LPC55S69_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_CURRENT_BINARY_DIR}/${LPC55S69_TARGET_NAME} ${LPC55S69_FLASH_BINARY}
    COMMENT "Generating binary file ${LPC55S69_FLASH_BINARY}"
)

# Create flash target for easy programming
add_custom_target(flash_lpc55s69
    COMMAND ${CMAKE_SOURCE_DIR}/tools/flash.sh ${LPC55S69_FLASH_BINARY}
    DEPENDS ${LPC55S69_TARGET_NAME}
    COMMENT "Flashing ${LPC55S69_FLASH_BINARY} to LPC55S69 board"
)

# Installation
install(TARGETS ${LPC55S69_TARGET_NAME}
    RUNTIME DESTINATION bin
)

# Add examples subdirectory - using relative path to avoid binary directory conflict
if(EXISTS ${CMAKE_SOURCE_DIR}/examples/dual_hid/lpc55s69)
    add_subdirectory(examples/dual_hid/lpc55s69 examples/dual_hid/lpc55s69)
endif()