# Dual HID Example for Hurricane USB Interface Manager

This example demonstrates a comprehensive implementation of Hurricane's dual USB stack capabilities on the imxrt1060 platform, showcasing simultaneous operation of both USB host and device functionality.

## Overview

The Dual HID example demonstrates:

1. Simultaneous operation of both USB controllers on the RT1060:
   * USB1 controller configured as a USB Host
   * USB2 controller configured as a USB Device

2. Composite USB Device implementation:
   * Presents both Mouse and Keyboard HID interfaces
   * Dynamically configures descriptors and endpoints
   * Generates periodic HID reports

3. USB Host implementation:
   * Detects and handles connected HID devices
   * Processes incoming HID reports
   * Extracts device information

4. Clean architecture with separated concerns:
   * Main coordinator (`main.c`)
   * Device-specific configuration (`device_config.c/h`)
   * Host-specific handling (`host_handler.c/h`)

## Architecture

This example uses a modular architecture to cleanly separate responsibilities and demonstrate best practices for implementing dual USB stack applications.

```
+----------------------------------------+
|              Application               |
|               (main.c)                 |
+----------------+---------------------+-+
                 |                     |
    +------------v----------+  +-------v--------------+
    |    Device Config      |  |     Host Handler     |
    | (device_config.c/h)   |  |  (host_handler.c/h)  |
    +------------+----------+  +----------+------------+
                 |                        |
    +------------v------------------------v------------+
    |           USB Interface Manager                  |
    |     (lib/hurricane/core/usb_interface_manager)   |
    +------------+------------------------+------------+
                 |                        |
    +------------v----------+  +----------v------------+
    |    Device HAL         |  |      Host HAL         |
    | (usb_hw_hal_device)   |  |  (usb_hw_hal_host)    |
    +------------+----------+  +----------+------------+
                 |                        |
    +------------v----------+  +----------v------------+
    |  USB2 Controller      |  |   USB1 Controller     |
    |     (Device)          |  |       (Host)          |
    +---------------------+-+  +-+---------------------+
                          |      |
                     +----v------v----+
                     |  RT1060 Board  |
                     +----------------+
```

## Components

### Main Application (`main.c`)

The main application coordinates both USB stacks and provides:
- Initialization of Hurricane core and USB stacks
- Configuration of both host and device modes
- Clean main loop handling both stacks
- Status reporting and diagnostics
- System startup and shutdown procedures

### Device Configuration (`device_config.c/h`)

The device configuration module handles all USB device-related functionality:
- Descriptor generation (device, configuration, HID reports)
- Dynamic interface and endpoint configuration
- HID report generation (mouse movements and keyboard keypresses)
- USB control request handling
- Connection state management

### Host Handler (`host_handler.c/h`)

The host handler module manages USB host-related functionality:
- USB device detection and enumeration
- HID device configuration and interaction
- Report reception and processing
- Device information extraction
- Callback registration for application notification

### USB Interface Manager

The core component that enables the dual-stack configuration:
- Manages interfaces dynamically at runtime
- Routes events between application modules and hardware
- Acts as intermediary for control transfers
- Handles interface registry for both host and device stacks

## Hardware Configuration

The RT1060 has two USB controllers that are used as follows:

1. **USB1 Controller (Host Mode)**
   - Connected to a USB Type-A connector
   - Detects and interacts with USB devices
   - Uses interrupt polling for report reception

2. **USB2 Controller (Device Mode)**
   - Connected to a USB Micro-B connector
   - Enumerates as a composite HID device
   - Sends periodic mouse and keyboard reports

## How It Works

### Device Mode Operation

1. The device controller initializes with basic device descriptors
2. When a host connects, interfaces are dynamically configured
3. Mouse and keyboard interfaces are added to the configuration
4. Endpoints are configured for each interface
5. Periodic reports are generated based on timers:
   - Mouse movements follow a circular pattern
   - Keyboard periodically types "HELLO"

### Host Mode Operation

1. The host controller actively polls for device connections
2. When a device is attached, the controller enumerates it
3. For HID devices, the appropriate protocol is configured
4. Interrupt endpoints are monitored for incoming reports
5. Reports are processed and made available to the application
6. Device information is extracted and displayed

### Coordination Between Modes

The main application coordinates both stacks:
- Both controllers run simultaneously and independently
- Status information from both stacks is consolidated
- The application can relay data between stacks if desired
- Resource contention is avoided through proper abstraction

## Building and Running

### Prerequisites

- NXP MCUXpresso SDK for MIMXRT1060
- ARM GCC Toolchain
- CMake 3.20 or higher
- USB cables for both host and device connections

### Build Instructions

```bash
# Create a build directory
mkdir -p build_rt1060 && cd build_rt1060

# Configure with CMake
cmake .. -DPLATFORM=rt1060 -DNXP_SDK_PATH=/path/to/nxp/sdk

# Build the example
make hurricane_dual_hid

# Flash to the board
make flash_dual_hid
```

### Test Setup

To test both sides of the dual USB functionality:

1. Connect the RT1060 board to your computer via the USB Micro-B port
   - Your computer should detect a new composite HID device
   - You should observe mouse cursor movements in a circular pattern
   - The keyboard should periodically type "HELLO"

2. Connect a USB HID device (e.g., a mouse or keyboard) to the USB Type-A port
   - The example will detect and display device information
   - Reports from the connected device will be processed
   - For keyboards, LED states can be controlled

## Debug Output

The example provides detailed debug logging through the serial console:

- System initialization and status information
- USB device connections and disconnections
- Interface configurations
- HID report generation and reception
- Error handling and diagnostics

## Code Structure

```
examples/rt1060/dual_hid/
├── CMakeLists.txt         - Build configuration
├── device_config.c        - Device mode implementation
├── device_config.h        - Device mode interface
├── host_handler.c         - Host mode implementation
├── host_handler.h         - Host mode interface
├── main.c                 - Main application
└── README.md              - Documentation
```

## Key Implementation Details

### Dynamic Interface Configuration

The example demonstrates dynamic interface configuration, allowing:
- Adding interfaces at runtime
- Configuring endpoints with specific properties
- Updating device descriptors
- Handling different HID report formats

### USB Host Enumeration

The host mode shows proper USB device enumeration:
- Standard device descriptor requests
- Configuration selection
- Interface claiming
- Endpoint configuration
- Protocol setting

### Thread Safety

The implementation ensures thread safety through:
- Proper mutex protection in the interface manager
- Clear ownership boundaries between components
- State tracking to prevent race conditions

## Extending the Example

This example can be extended in several ways:

1. **Add More Device Classes**
   - CDC for serial communication
   - Mass Storage for file transfer
   - Audio for sound input/output

2. **Enhanced Host Capabilities**
   - Support for multiple simultaneous devices
   - Class-specific functionality (printer, storage, etc.)
   - Hub support for connecting multiple devices

3. **Advanced Features**
   - Data translation between connected devices
   - Customizable HID report generation
   - Configuration interface over USB

## Troubleshooting

Common issues and solutions:

1. **Device Not Detected**
   - Check USB cable connections
   - Verify power supply is adequate
   - Ensure USB controllers are properly initialized

2. **Build Errors**
   - Verify NXP SDK path is correct
   - Check if required libraries are installed
   - Update toolchain to compatible version

3. **HID Reports Not Working**
   - Check endpoint configuration
   - Verify report descriptor format
   - Ensure proper interrupt timing

## Related Documentation

- [Hurricane USB Interface Manager API](../../lib/hurricane/core/usb_interface_manager.h)
- [RT1060 USB HAL Implementation](../../lib/hurricane/hw/boards/rt1060/)
- [Dual USB Architecture Plan](../../dual_usb_architecture_plan.md)
- [Dual USB Stack Implementation](../../dual_usb_stack_implementation.md)