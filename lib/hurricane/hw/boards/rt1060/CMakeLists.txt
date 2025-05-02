# RT1060 Hardware Abstraction Layer for Hurricane USB Stack
# This builds both host and device USB HAL implementations for RT1060

# Include NXP SDK integration
include(${CMAKE_SOURCE_DIR}/cmake/nxp_sdk.cmake)

add_library(hurricane_rt1060_hal
    usb_hw_hal_host_rt1060.c
    usb_hw_hal_device_rt1060.c
    startup.c
)

# Add NXP SDK components for dual USB stack
add_nxp_sdk_components(hurricane_rt1060_hal
    USB_HOST
    USB_DEVICE
    USB_HID_HOST
    USB_HID_DEVICE
)

# Generate NXP SDK configuration headers
generate_nxp_sdk_config(hurricane_rt1060_hal)

target_include_directories(hurricane_rt1060_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..  # For lib/hurricane includes
)

target_link_libraries(hurricane_rt1060_hal PUBLIC
    hurricane_core
)

# Create the main executable target
add_executable(hurricane_rt1060
    ${CMAKE_CURRENT_SOURCE_DIR}/startup.c
)

target_link_libraries(hurricane_rt1060
    hurricane_rt1060_hal
    hurricane_core
)

# Linker script and flags
target_link_options(hurricane_rt1060 PRIVATE
    "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
)

# Install the library and headers
install(TARGETS hurricane_rt1060_hal
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
