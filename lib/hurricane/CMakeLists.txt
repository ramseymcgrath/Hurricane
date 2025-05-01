set(COMPONENT_SRCS
    usb/usb_control.c
    usb/usb_hid.c
    core/hurricane.c
)

# Select board HAL automatically if building with IDF
if(IDF_BUILD)
    list(APPEND COMPONENT_SRCS
        hw/boards/esp32/usb_hw_hal_esp32.c
    )
endif()

set(COMPONENT_ADD_INCLUDEDIRS
    .
    usb
    core
    hw
)

register_component()
