# Define common source files
set(HURRICANE_SRC_FILES
    usb/usb_control.c
    usb/usb_hid.c
    core/hurricane.c
)

# Define include directories
set(HURRICANE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/usb
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/hw
)

# For ESP-IDF builds, use the component registration system
if(IDF_BUILD)
    set(COMPONENT_SRCS ${HURRICANE_SRC_FILES})
    list(APPEND COMPONENT_SRCS
        hw/boards/esp32/usb_hw_hal_esp32.c
    )
    set(COMPONENT_ADD_INCLUDEDIRS
        .
        usb
        core
        hw
    )
    register_component()
else()
    # For standard CMake builds (RT1060, LPC55S69, etc.), use standard CMake commands
    add_library(hurricane ${HURRICANE_SRC_FILES})
    target_include_directories(hurricane PUBLIC ${HURRICANE_INCLUDE_DIRS})
endif()
