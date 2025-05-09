# Define source files for hurricane library
set(HURRICANE_SRCS
    usb/usb_control.c
    usb/usb_hid.c
    core/hurricane_usb.c
)

# Select board HAL automatically if building with IDF
if(IDF_BUILD)
    list(APPEND HURRICANE_SRCS
        hw/boards/esp32/usb_hw_hal_esp32.c
    )

    # Define include directories for ESP-IDF component
    set(COMPONENT_SRCS ${HURRICANE_SRCS})
    set(COMPONENT_ADD_INCLUDEDIRS
        .
        usb
        core
        hw
    )
    
    # Register as ESP-IDF component
    register_component()
else()
    # For non-ESP platforms like lpc55s69, use standard CMake
    
    # Create hurricane static library
    add_library(hurricane STATIC ${HURRICANE_SRCS})
    
    # Add include directories
    target_include_directories(hurricane PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/usb
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/hw
        ../../../examples/lpc55s69/dual_hid
    )
    
    # Add NXP SDK include paths for LPC55S69 platform
    if(DEFINED PLATFORM AND PLATFORM STREQUAL "lpc55s69")
        target_include_directories(hurricane PUBLIC
            ${NXP_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers
            ${NXP_SDK_PATH}/devices/LPC/LPC5500/LPC55S69
            ${NXP_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/utilities
            ${NXP_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/components
            ${NXP_SDK_PATH}/CMSIS/Core/Include
            ${NXP_SDK_PATH}/drivers/common
            ${NXP_SDK_PATH}/components/gpio
            ${NXP_SDK_PATH}/components/lists
            ${NXP_SDK_PATH}/components/uart
            ${NXP_SDK_PATH}/components/serial_manager
        )
        
        # Add required CPU and device definitions for LPC55S69
        target_compile_definitions(hurricane PUBLIC
            CPU_LPC55S69JBD100_cm33_core0
            __CORTEX_M=33
            __FPU_PRESENT=1
            __NEWLIB__
            __USE_CMSIS
        )
    endif()
endif()
