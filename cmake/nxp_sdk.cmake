# NXP SDK Integration for Hurricane USB Library
#
# This script finds and configures the NXP SDK components for the Hurricane dual USB stack.
# It handles path configuration, component selection, and build flags for RT1060.

# Options for SDK configuration
option(ENABLE_USB_HOST "Enable USB Host stack" ON)
option(ENABLE_USB_DEVICE "Enable USB Device stack" ON)
option(ENABLE_DUAL_USB "Enable dual USB stack (Host and Device simultaneously)" ON)

# Modify SDK path to point to the correct location
set(NXP_SDK_PATH "/Users/ramseymcgrath/code/mcuxpresso-sdk/mcuxsdk" CACHE PATH "Path to NXP MCUXpresso SDK")

# Package options
option(USE_PACKAGED_SDK "Use a packaged version of the NXP SDK" OFF)
set(NXP_SDK_PACKAGE_PATH "" CACHE PATH "Path to packaged NXP SDK (if not using direct SDK)")

# Determine SDK path based on user options
if(USE_PACKAGED_SDK)
  if(NOT NXP_SDK_PACKAGE_PATH)
    message(FATAL_ERROR "NXP_SDK_PACKAGE_PATH is not set. Please provide the path to the packaged NXP SDK.")
  endif()
  set(EFFECTIVE_SDK_PATH ${NXP_SDK_PACKAGE_PATH})
else()
  if(NOT NXP_SDK_PATH)
    message(FATAL_ERROR "NXP_SDK_PATH is not set. Please provide the path to the NXP MCUXpresso SDK.")
  endif()
  set(EFFECTIVE_SDK_PATH ${NXP_SDK_PATH})
endif()

# Verify SDK path exists
if(NOT EXISTS ${EFFECTIVE_SDK_PATH})
  message(FATAL_ERROR "NXP SDK path '${EFFECTIVE_SDK_PATH}' does not exist.")
endif()

# SDK version check with better error handling
set(REQUIRED_SDK_VERSION "2.10.0")
if(EXISTS "~/code/mcuxsdk-middleware-usb/version.txt")
  file(READ "~/code/mcuxsdk-middleware-usb/version.txt" SDK_VERSION)
  string(STRIP "${SDK_VERSION}" SDK_VERSION)
  message(STATUS "Found NXP SDK version: ${SDK_VERSION}")
  
  # Version check logic
  if(SDK_VERSION VERSION_LESS REQUIRED_SDK_VERSION)
    message(WARNING "NXP SDK version ${SDK_VERSION} is older than recommended version ${REQUIRED_SDK_VERSION}. Some features might not work correctly.")
  endif()
else()
  message(WARNING "Could not determine NXP SDK version. Continuing anyway.")
endif()

# Set USB middleware path to custom location
set(NXP_USB_MIDDLEWARE_PATH "/Users/ramseymcgrath/code/mcuxsdk-middleware-usb")
list(APPEND NXP_SDK_INCLUDE_DIRS
  ${NXP_USB_MIDDLEWARE_PATH}/device/include
  ${NXP_USB_MIDDLEWARE_PATH}/host/include
)

# Set SDK include directories with more comprehensive paths for the selected device
if(DEFINED HURRICANE_TARGET_DEVICE)
  if(HURRICANE_TARGET_DEVICE STREQUAL "MIMXRT1062")
    set(NXP_SDK_INCLUDE_DIRS
      ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060
      ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/drivers
      ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/utilities/debug_console
      ${NXP_USB_MIDDLEWARE_PATH}
      ${NXP_USB_MIDDLEWARE_PATH}/include
      ${NXP_USB_MIDDLEWARE_PATH}/phy
      ${NXP_USB_MIDDLEWARE_PATH}/device
      ${NXP_USB_MIDDLEWARE_PATH}/host
      ${NXP_USB_MIDDLEWARE_PATH}/device/class
      ${NXP_USB_MIDDLEWARE_PATH}/host/class
      ${EFFECTIVE_SDK_PATH}/CMSIS/Core/Include
      ${EFFECTIVE_SDK_PATH}/components/uart
      ${EFFECTIVE_SDK_PATH}/components/serial_manager
      ${EFFECTIVE_SDK_PATH}/components/lists
      ${EFFECTIVE_SDK_PATH}/drivers/common
      ${EFFECTIVE_SDK_PATH}/drivers/gpio
      )
    
  elseif(HURRICANE_TARGET_DEVICE STREQUAL "LPC55S69")
    set(NXP_SDK_INCLUDE_DIRS
      ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69
      ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers
      ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/utilities
      ${NXP_USB_MIDDLEWARE_PATH}
      ${NXP_USB_MIDDLEWARE_PATH}/include
      ${NXP_USB_MIDDLEWARE_PATH}/phy
      ${NXP_USB_MIDDLEWARE_PATH}/device
      ${NXP_USB_MIDDLEWARE_PATH}/host
      ${EFFECTIVE_SDK_PATH}/components/osa
      ${EFFECTIVE_SDK_PATH}/components/usb
      ${EFFECTIVE_SDK_PATH}/components/usb/device
      ${EFFECTIVE_SDK_PATH}/components/usb/phy
      ${NXP_USB_MIDDLEWARE_PATH}/device/class
      ${NXP_USB_MIDDLEWARE_PATH}/host/class
      ${EFFECTIVE_SDK_PATH}/CMSIS/Core/Include
      ${EFFECTIVE_SDK_PATH}/CMSIS/Driver/Include
      ${EFFECTIVE_SDK_PATH}/components/uart
      ${EFFECTIVE_SDK_PATH}/components/serial_manager
      ${EFFECTIVE_SDK_PATH}/components/lists
      ${EFFECTIVE_SDK_PATH}/drivers/common
      ${EFFECTIVE_SDK_PATH}/drivers/gpio
    )
    
    # Store the linker script path for LPC55S69 for later use
    set(LPC55S69_LINKER_SCRIPT "${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/gcc/LPC55S69_cm33_core0_flash.ld" CACHE INTERNAL "LPC55S69 linker script")
    if(NOT EXISTS ${LPC55S69_LINKER_SCRIPT})
      set(LPC55S69_LINKER_SCRIPT "${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/gcc/LPC55S69_cm33_core0_ram.ld" CACHE INTERNAL "LPC55S69 linker script")
    endif()
    if(NOT EXISTS ${LPC55S69_LINKER_SCRIPT})
      message(WARNING "LPC55S69 linker script not found")
    endif()
  else()
    message(FATAL_ERROR "Unsupported HURRICANE_TARGET_DEVICE: ${HURRICANE_TARGET_DEVICE}")
  endif()
else()
  # Default to RT1062 if no device is specified
  set(HURRICANE_TARGET_DEVICE "MIMXRT1062")
  set(NXP_SDK_INCLUDE_DIRS
    ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060
    ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/drivers
    ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/utilities/debug_console
    ${NXP_USB_MIDDLEWARE_PATH}
    ${NXP_USB_MIDDLEWARE_PATH}/include
    ${NXP_USB_MIDDLEWARE_PATH}/phy
    ${NXP_USB_MIDDLEWARE_PATH}/device
    ${NXP_USB_MIDDLEWARE_PATH}/host
    ${NXP_USB_MIDDLEWARE_PATH}/device/class
    ${NXP_USB_MIDDLEWARE_PATH}/host/class
    ${EFFECTIVE_SDK_PATH}/CMSIS/Core/Include
    ${EFFECTIVE_SDK_PATH}/components/uart
    ${EFFECTIVE_SDK_PATH}/components/serial_manager
    ${EFFECTIVE_SDK_PATH}/components/lists
    ${EFFECTIVE_SDK_PATH}/drivers/common
    ${EFFECTIVE_SDK_PATH}/drivers/gpio
  )
endif()

# Set SDK core system sources based on target device
if(HURRICANE_TARGET_DEVICE STREQUAL "MIMXRT1062")
  set(NXP_SDK_CORE_SOURCES
    ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/system_MIMXRT1062.c
    ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/drivers/fsl_clock.c
    ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/drivers/fsl_gpio.c
    ${EFFECTIVE_SDK_PATH}/drivers/common/fsl_common.c
    ${EFFECTIVE_SDK_PATH}/drivers/common/fsl_common_arm.c
    ${EFFECTIVE_SDK_PATH}/drivers/gpio/fsl_gpio.c
  )
elseif(HURRICANE_TARGET_DEVICE STREQUAL "LPC55S69")
  set(NXP_SDK_CORE_SOURCES
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/system_LPC55S69_cm33_core0.c
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers/fsl_clock.c
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers/fsl_power.c
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers/fsl_reset.c
    ${EFFECTIVE_SDK_PATH}/drivers/common/fsl_common.c
    ${EFFECTIVE_SDK_PATH}/drivers/common/fsl_common_arm.c
    ${EFFECTIVE_SDK_PATH}/drivers/gpio/fsl_gpio.c
  )
endif()

# Set SDK USB common sources
set(NXP_SDK_USB_COMMON_SOURCES
  ${NXP_USB_MIDDLEWARE_PATH}/phy/usb_phy.c
  ${NXP_USB_MIDDLEWARE_PATH}/device/usb_device_dci.c
)

# Set SDK USB device sources
set(NXP_SDK_USB_DEVICE_SOURCES
  ${NXP_USB_MIDDLEWARE_PATH}/device/usb_device_ch9.c
  ${NXP_USB_MIDDLEWARE_PATH}/device/class/usb_device_class.c
)

# Add device controller based on target
if(HURRICANE_TARGET_DEVICE STREQUAL "MIMXRT1062")
  list(APPEND NXP_SDK_USB_DEVICE_SOURCES
    ${NXP_USB_MIDDLEWARE_PATH}/device/usb_device_ehci.c
  )
elseif(HURRICANE_TARGET_DEVICE STREQUAL "LPC55S69")
  list(APPEND NXP_SDK_USB_DEVICE_SOURCES
    ${NXP_USB_MIDDLEWARE_PATH}/device/usb_device_lpcip3511.c
#    ${NXP_USB_MIDDLEWARE_PATH}/device/usb_device_lpcip3511hs.c
  )
  # Store this definition for later use with targets
  set(LPC55S69_DEVICE_DEFINITIONS USB_DEVICE_CONFIG_LPCIP3511HS=1)
endif()

# Set SDK USB host sources
set(NXP_SDK_USB_HOST_SOURCES
#  ${NXP_USB_MIDDLEWARE_PATH}/host/usb_host.c
  ${NXP_USB_MIDDLEWARE_PATH}/host/usb_host_devices.c
  ${NXP_USB_MIDDLEWARE_PATH}/host/usb_host_ehci.c
  ${NXP_USB_MIDDLEWARE_PATH}/host/usb_host_framework.c
  ${NXP_USB_MIDDLEWARE_PATH}/host/class/usb_host_hub.c
  ${NXP_USB_MIDDLEWARE_PATH}/host/class/usb_host_hub_app.c
)

# Set SDK USB HID class sources
set(NXP_SDK_USB_HID_HOST_SOURCES
  ${NXP_USB_MIDDLEWARE_PATH}/host/class/usb_host_hid.c
)

set(NXP_SDK_USB_HID_DEVICE_SOURCES
  ${NXP_USB_MIDDLEWARE_PATH}/device/class/usb_device_hid.c
)

# Additional SDK components for different targets
if(HURRICANE_TARGET_DEVICE STREQUAL "MIMXRT1062")
  set(NXP_SDK_GPIO_SOURCES
    ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/drivers/fsl_gpio.c
  )

  set(NXP_SDK_CLOCK_SOURCES
    ${EFFECTIVE_SDK_PATH}/devices/RT/RT1060/drivers/fsl_clock.c
  )
elseif(HURRICANE_TARGET_DEVICE STREQUAL "LPC55S69")

  set(NXP_SDK_CLOCK_SOURCES
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers/fsl_clock.c
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers/fsl_power.c
  )
endif()

# Function to add NXP SDK components to a target
function(add_nxp_sdk_components TARGET)
  # Add include directories
  target_include_directories(${TARGET} PRIVATE ${NXP_SDK_INCLUDE_DIRS})
  
  # Define variables to track which components to add
  set(NEED_USB_HOST FALSE)
  set(NEED_USB_DEVICE FALSE)
  set(NEED_USB_HID_HOST FALSE)
  set(NEED_USB_HID_DEVICE FALSE)
  set(NEED_GPIO FALSE)
  set(NEED_CLOCK FALSE)
  
  # Parse arguments to determine which components to add
  foreach(COMPONENT ${ARGN})
    if(COMPONENT STREQUAL "USB_HOST")
      set(NEED_USB_HOST TRUE)
    elseif(COMPONENT STREQUAL "USB_DEVICE")
      set(NEED_USB_DEVICE TRUE)
    elseif(COMPONENT STREQUAL "USB_HID_HOST")
      set(NEED_USB_HID_HOST TRUE)
      set(NEED_USB_HOST TRUE)  # HID Host requires Host
    elseif(COMPONENT STREQUAL "USB_HID_DEVICE")
      set(NEED_USB_HID_DEVICE TRUE)
      set(NEED_USB_DEVICE TRUE)  # HID Device requires Device
    elseif(COMPONENT STREQUAL "GPIO")
      set(NEED_GPIO TRUE)
    elseif(COMPONENT STREQUAL "CLOCK")
      set(NEED_CLOCK TRUE)
    endif()
  endforeach()
  
  # Create a list of source files to add
  set(COMPONENT_SOURCES)
  
  # Always add core components
  list(APPEND COMPONENT_SOURCES ${NXP_SDK_CORE_SOURCES})
  
  # Always add common USB components
  list(APPEND COMPONENT_SOURCES ${NXP_SDK_USB_COMMON_SOURCES})
  
  # Add USB Host components if needed
  if(NEED_USB_HOST AND ENABLE_USB_HOST)
    list(APPEND COMPONENT_SOURCES ${NXP_SDK_USB_HOST_SOURCES})
    target_compile_definitions(${TARGET} PRIVATE USB_HOST_CONFIG_EHCI=1)
  endif()
  
  # Add USB Device components if needed
  if(NEED_USB_DEVICE AND ENABLE_USB_DEVICE)
    list(APPEND COMPONENT_SOURCES ${NXP_SDK_USB_DEVICE_SOURCES})
    target_compile_definitions(${TARGET} PRIVATE USB_DEVICE_CONFIG_EHCI=1)
  endif()
  
  # Add HID Host component if needed
  if(NEED_USB_HID_HOST AND ENABLE_USB_HOST)
    list(APPEND COMPONENT_SOURCES ${NXP_SDK_USB_HID_HOST_SOURCES})
    target_compile_definitions(${TARGET} PRIVATE USB_HOST_CONFIG_HID=1)
  endif()
  
  # Add HID Device component if needed
  if(NEED_USB_HID_DEVICE AND ENABLE_USB_DEVICE)
    list(APPEND COMPONENT_SOURCES ${NXP_SDK_USB_HID_DEVICE_SOURCES})
    target_compile_definitions(${TARGET} PRIVATE USB_DEVICE_CONFIG_HID=1)
  endif()
  
  # Add GPIO if needed
  if(NEED_GPIO)
    list(APPEND COMPONENT_SOURCES ${NXP_SDK_GPIO_SOURCES})
  endif()
  
  # Add CLOCK if needed
  if(NEED_CLOCK)
    list(APPEND COMPONENT_SOURCES ${NXP_SDK_CLOCK_SOURCES})
  endif()
  
  # Add all source files to the target
  target_sources(${TARGET} PRIVATE ${COMPONENT_SOURCES})
  
  # Add necessary compiler definitions for the specified device
  if(HURRICANE_TARGET_DEVICE STREQUAL "MIMXRT1062")
    target_compile_definitions(${TARGET} PRIVATE
      CPU_MIMXRT1062DVL6A
      USB_STACK_BM
      SDK_DEVICE_FAMILY=MIMXRT1062
      FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1
      PRINTF_ADVANCED_ENABLE=1
    )
  elseif(HURRICANE_TARGET_DEVICE STREQUAL "LPC55S69")
    target_compile_definitions(${TARGET} PRIVATE
      CPU_LPC55S69JBD100_cm33_core0
      USB_STACK_BM
      SDK_DEVICE_FAMILY=LPC55S69
      FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1
      PRINTF_ADVANCED_ENABLE=1
      USB_DEVICE_CONFIG_CDC_MULTIPORT=1
      FSL_OSA_BM_TASK_ENABLE=0
      FSL_OSA_BM_TIMER_CONFIG=0
      USB_DEVICE_CONFIG_LPCIP3511HS=1
      USB_DEVICE_CONFIG_HIGHSPEED=1
      USB_DEVICE_CONFIG_DETACH_ENABLE=1
    )
  endif()
  
  # Set SDK configurations based on dual USB stack option
  if(ENABLE_USB_HOST AND ENABLE_USB_DEVICE AND ENABLE_DUAL_USB)
    # Dual USB stack configuration
    target_compile_definitions(${TARGET} PRIVATE
      USB_DUAL_STACK=1
      USB_DEVICE_CONFIG_EHCI=1
      USB_HOST_CONFIG_EHCI=1
      HURRICANE_DUAL_USB_SUPPORT=1
    )
  elseif(ENABLE_USB_HOST)
    # Host-only configuration
    target_compile_definitions(${TARGET} PRIVATE
      USB_DUAL_STACK=0
      USB_HOST_CONFIG_EHCI=1
    )
  elseif(ENABLE_USB_DEVICE)
    # Device-only configuration
    target_compile_definitions(${TARGET} PRIVATE
      USB_DUAL_STACK=0
      USB_DEVICE_CONFIG_EHCI=1
    )
  endif()
  
  message(STATUS "Configured NXP SDK components for target ${TARGET}")
endfunction()

# Function to generate NXP SDK configuration headers with more options
function(generate_nxp_sdk_config TARGET)
  # Create directory for generated files
  set(CONFIG_DIR ${CMAKE_CURRENT_BINARY_DIR}/nxp_sdk_config)
  file(MAKE_DIRECTORY ${CONFIG_DIR})
  
  # USB device configuration with more options
  set(USB_DEVICE_CONFIG_FILE ${CONFIG_DIR}/usb_device_config.h)
  file(WRITE ${USB_DEVICE_CONFIG_FILE} "/* Generated USB device configuration for ${TARGET} */\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#ifndef USB_DEVICE_CONFIG_H\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_H\n\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_EHCI (1U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_MAX_POWER (500U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_SELF_POWER (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_DETACH_ENABLE (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_ENDPOINTS (8U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_HID (1U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_CDC_ACM (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_MSC (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_AUDIO (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_PHDC (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_VIDEO (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_BUFFER_PROPERTY_CACHEABLE (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "#define USB_DEVICE_CONFIG_CHARGER_DETECT (0U)\n")
  file(APPEND ${USB_DEVICE_CONFIG_FILE} "\n#endif /* USB_DEVICE_CONFIG_H */\n")
  
  # USB host configuration with more options
  set(USB_HOST_CONFIG_FILE ${CONFIG_DIR}/usb_host_config.h)
  file(WRITE ${USB_HOST_CONFIG_FILE} "/* Generated USB host configuration for ${TARGET} */\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#ifndef USB_HOST_CONFIG_H\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_H\n\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_EHCI (1U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_MAX_POWER (500U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_MAX_INTERFACES (5U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_MAX_ENDPOINTS (8U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_MAX_DEVICES (8U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_HID (1U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_HUB (0U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_CDC (0U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_MSC (0U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "#define USB_HOST_CONFIG_BUFFER_PROPERTY_CACHEABLE (0U)\n")
  file(APPEND ${USB_HOST_CONFIG_FILE} "\n#endif /* USB_HOST_CONFIG_H */\n")
  
  # Physical layer (PHY) configuration
  set(USB_PHY_CONFIG_FILE ${CONFIG_DIR}/usb_phy_config.h)
  file(WRITE ${USB_PHY_CONFIG_FILE} "/* Generated USB PHY configuration for ${TARGET} */\n")
  file(APPEND ${USB_PHY_CONFIG_FILE} "#ifndef USB_PHY_CONFIG_H\n")
  file(APPEND ${USB_PHY_CONFIG_FILE} "#define USB_PHY_CONFIG_H\n\n")
  file(APPEND ${USB_PHY_CONFIG_FILE} "#include \"fsl_usb_phy.h\"\n")
  file(APPEND ${USB_PHY_CONFIG_FILE} "#define BOARD_USB_PHY_D_CAL USB_PHY_D_CAL_DEFAULT\n")
  file(APPEND ${USB_PHY_CONFIG_FILE} "#define BOARD_USB_PHY_TXCAL45DP USB_PHY_TXCAL45DP_DEFAULT\n")
  file(APPEND ${USB_PHY_CONFIG_FILE} "#define BOARD_USB_PHY_TXCAL45DM USB_PHY_TXCAL45DM_DEFAULT\n")
  file(APPEND ${USB_PHY_CONFIG_FILE} "\n#endif /* USB_PHY_CONFIG_H */\n")
  
  # Add config directory to include paths
  target_include_directories(${TARGET} PRIVATE ${CONFIG_DIR})
  
  message(STATUS "Generated NXP SDK configuration headers for ${TARGET}")
endfunction()

# Macro to find SDK components and verify existence
macro(find_sdk_component COMPONENT_NAME COMPONENT_PATH RESULT_VAR)
  set(FULL_PATH "${EFFECTIVE_SDK_PATH}/${COMPONENT_PATH}")
  if(EXISTS "${FULL_PATH}")
    set(${RESULT_VAR} TRUE)
  else()
    set(${RESULT_VAR} FALSE)
    message(WARNING "NXP SDK component ${COMPONENT_NAME} not found at ${FULL_PATH}")
  endif()
endmacro()

# Verify critical SDK components exist
find_sdk_component("USB Device Stack" "middleware/usb/device/usb_device_dci.c" USB_DEVICE_FOUND)
find_sdk_component("USB Host Stack" "middleware/usb/host/usb_host_hci.c" USB_HOST_FOUND)
find_sdk_component("USB HID Device Class" "middleware/usb/device/class/usb_device_hid.c" USB_HID_DEVICE_FOUND)
find_sdk_component("USB HID Host Class" "middleware/usb/host/class/usb_host_hid.c" USB_HID_HOST_FOUND)

# Add missing component paths for LPC55S69
list(APPEND NXP_SDK_INCLUDE_DIRS
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/drivers
    ${EFFECTIVE_SDK_PATH}/devices/LPC/LPC5500/LPC55S69/utilities
)

if(NOT USB_DEVICE_FOUND AND ENABLE_USB_DEVICE)
  message(WARNING "USB Device stack not found but ENABLE_USB_DEVICE is ON. Device functionality might not work.")
endif()

if(NOT USB_HOST_FOUND AND ENABLE_USB_HOST)
  message(WARNING "USB Host stack not found but ENABLE_USB_HOST is ON. Host functionality might not work.")
endif()

message(STATUS "NXP SDK integration configured: ${EFFECTIVE_SDK_PATH}")
message(STATUS "Dual USB stack: ${ENABLE_DUAL_USB}")
message(STATUS "USB Host enabled: ${ENABLE_USB_HOST}")
message(STATUS "USB Device enabled: ${ENABLE_USB_DEVICE}")