# RT1060 target with dual USB stack support
cmake_minimum_required(VERSION 3.20)

# Set target device and include NXP SDK integration script
set(HURRICANE_TARGET_DEVICE "MIMXRT1062" CACHE STRING "NXP MCU device name")
include(${CMAKE_SOURCE_DIR}/cmake/nxp_sdk.cmake)

# Source files for the RT1060 target
file(GLOB MAIN_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

# RT1060 target specific definitions
set(RT1060_TARGET_NAME "hurricane_rt1060")
set(RT1060_FLASH_BINARY "${CMAKE_CURRENT_BINARY_DIR}/${RT1060_TARGET_NAME}.bin")

# Create RT1060 executable with main sources
add_executable(${RT1060_TARGET_NAME}
    ${MAIN_SRC}
)

# Include directories
target_include_directories(${RT1060_TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/hurricane
    ${CMAKE_SOURCE_DIR}/lib/hurricane/core
    ${CMAKE_SOURCE_DIR}/lib/hurricane/hw
    ${CMAKE_SOURCE_DIR}/lib/hurricane/usb
)

# Link against our RT1060 HAL library and Hurricane core
target_link_libraries(${RT1060_TARGET_NAME}
    hurricane_rt1060_hal
    hurricane
)

# Linker script
target_link_options(${RT1060_TARGET_NAME} PRIVATE
    "-T${CMAKE_SOURCE_DIR}/lib/hurricane/hw/boards/rt1060/linker.ld"
)

# Add NXP SDK components with more comprehensive list
add_nxp_sdk_components(${RT1060_TARGET_NAME}
    USB_HOST
    USB_DEVICE
    USB_HID_HOST
    USB_HID_DEVICE
    GPIO
    CLOCK
)

# Generate NXP SDK configuration headers
generate_nxp_sdk_config(${RT1060_TARGET_NAME})

# RT1060-specific compiler options
target_compile_options(${RT1060_TARGET_NAME} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-d16
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
    -fno-common
    -fmessage-length=0
    -Wno-unused-parameter
    -Wno-unused-variable
    -O2
)

# RT1060-specific compiler definitions
target_compile_definitions(${RT1060_TARGET_NAME} PRIVATE
    CPU_MIMXRT1062DVL6A
    USB_STACK_BM
    HURRICANE_RT1060
    DUAL_USB_SUPPORT
    __NEWLIB__
    __USE_CMSIS
    NDEBUG
)

# RT1060-specific linker options
target_link_options(${RT1060_TARGET_NAME} PRIVATE
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
    -Wl,--print-memory-usage
)

# Binary file generation
add_custom_command(TARGET ${RT1060_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_CURRENT_BINARY_DIR}/${RT1060_TARGET_NAME} ${RT1060_FLASH_BINARY}
    COMMENT "Generating binary file ${RT1060_FLASH_BINARY}"
)

# Create flash target for easy programming
add_custom_target(flash_rt1060
    COMMAND ${CMAKE_SOURCE_DIR}/tools/flash.sh ${RT1060_FLASH_BINARY}
    DEPENDS ${RT1060_TARGET_NAME}
    COMMENT "Flashing ${RT1060_FLASH_BINARY} to RT1060 board"
)

# Add target to build examples
add_custom_target(build_examples
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target hurricane_dual_hid
    COMMENT "Building RT1060 examples"
)

# Installation
install(TARGETS ${RT1060_TARGET_NAME}
    RUNTIME DESTINATION bin
)

# Add examples subdirectory conditionally
if(EXISTS ${CMAKE_SOURCE_DIR}/examples/rt1060)
    add_subdirectory(${CMAKE_SOURCE_DIR}/examples/rt1060 ${CMAKE_BINARY_DIR}/examples/rt1060)
endif()
