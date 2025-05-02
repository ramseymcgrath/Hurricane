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
    )
endif()
