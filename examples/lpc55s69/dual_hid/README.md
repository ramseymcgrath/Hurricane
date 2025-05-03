# Dual HID Example for LPC55S69 with Hurricane USB Interface Manager

This example demonstrates a comprehensive implementation of Hurricane's dual USB stack capabilities on the LPC55S69 platform, showcasing simultaneous operation of both USB host and device functionality.

## Overview

The LPC55S69 Dual HID example demonstrates:

1. Simultaneous operation of both USB controllers on the LPC55S69:
   * USB0 controller configured as a USB Device (Full Speed IP3511)
   * USB1 controller configured as a USB Host (High Speed EHCI)

2. Composite USB Device implementation:
   * Presents both Mouse and Keyboard HID interfaces
   * Dynamically configures descriptors and endpoints
   * Generates periodic HID reports (mouse movement and keyboard input)

3. USB Host implementation:
   * Detects and handles connected HID devices
   * Processes incoming HID reports
   * Extracts device information
   * Supports HID keyboard LED status synchronization

4. Clean architecture with separated concerns:
   * Main coordinator (`main.c`)
   * Device-specific configuration (`device_config.c/h`)
   * Host-specific handling (`host_handler.c/h`)
   * Bidirectional communication between host and device stacks

## Architecture

This example uses a modular architecture to cleanly separate responsibilities and demonstrate best practices for implementing dual USB stack applications on the LPC55S69 platform.

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
    |  USB0 Controller      |  |   USB1 Controller     |
    |  (FS IP3511 Device)   |  |   (HS EHCI Host)      |
    +---------------------+-+  +-+---------------------+
                          |      |
                     +----v------v----+
                     | LPC55S69 Board |
                     +----------------+
```

## USB Controller Architecture

The LPC55S69 features a dual USB controller architecture that makes it particularly well-suited for implementing simultaneous USB host and device functionality:

### USB0 Controller (Device Mode)
- Full-Speed (12 Mbps) IP3511 USB controller
- Dedicated to device functionality
- Features hardware endpoint management
- Supports multiple interface configurations
- Connected to USB device connector (typically micro USB)

### USB1 Controller (Host Mode)
- High-Speed (480 Mbps) EHCI USB controller
- Dedicated to host functionality
- Provides standard EHCI host capabilities
- Supports multiple device detection and enumeration
- Connected to USB Type-A connector for peripheral devices

This physically separate controller architecture is different from microcontrollers that use a shared controller (like RT1060), making the implementation cleaner by avoiding the need for time-multiplexing and complex resource sharing.

## Components

### Main Application (`main.c`)

The main application coordinates both USB stacks and provides:
- Initialization of Hurricane core and USB stacks
- Configuration of both host and device modes
- Clean main loop handling both stacks
- Status reporting and diagnostics
- Bidirectional data relay between host and device stacks
- Keyboard LED state synchronization

### Device Configuration (`device_config.c/h`)

The device configuration module handles all USB device-related functionality:
- Descriptor generation (device, configuration, HID reports)
- Dynamic interface and endpoint configuration
- HID report generation (mouse movements and keyboard keypresses)
- USB control request handling
- Connection state management
- LED status reporting for keyboards

### Host Handler (`host_handler.c/h`)

The host handler module manages USB host-related functionality:
- USB device detection and enumeration
- HID device configuration and interaction
- Report reception and processing
- Device information extraction
- Callback registration for application notification
- Keyboard LED status synchronization

## Hardware Configuration

The LPC55S69 has two USB controllers that are used as follows:

1. **USB1 Controller (Host Mode)**
   - EHCI High-Speed controller
   - Connected to a USB Type-A connector
   - Detects and interacts with USB devices
   - Uses interrupt polling for report reception

2. **USB0 Controller (Device Mode)**
   - IP3511 Full-Speed controller
   - Connected to a USB Micro-B connector
   - Enumerates as a composite HID device
   - Sends periodic mouse and keyboard reports

## How It Works

### Device Mode Operation

1. The device controller (USB0) initializes with basic device descriptors
2. When a host connects, interfaces are dynamically configured
3. Mouse and keyboard interfaces are added to the configuration
4. Endpoints are configured for each interface
5. Periodic reports are generated based on timers:
   - Mouse movements follow a circular pattern
   - Keyboard periodically types "HELLO"

### Host Mode Operation

1. The host controller (USB1) actively polls for device connections
2. When a device is attached, the controller enumerates it
3. For HID devices, the appropriate protocol is configured
4. Interrupt endpoints are monitored for incoming reports
5. Reports are processed and made available to the application
6. Device information is extracted and displayed

### Bidirectional Communication Mechanism

One of the key features of this example is the bidirectional communication between the host and device stacks:

1. **HID Reports Forwarding**
   - When a HID device (mouse or keyboard) is connected to the host controller (USB1)
   - The host handler processes incoming reports
   - The main application forwards these reports to the device controller (USB0)
   - The device controller sends the reports to the computer connected to it
   - This effectively turns the LPC55S69 into a USB HID proxy

2. **Keyboard LED Status Synchronization**
   - When the computer connected to the device mode (USB0) changes keyboard LED states (Num/Caps/Scroll Lock)
   - The device controller receives these changes through SET_REPORT requests
   - A callback in main.c forwards these LED states to the host controller
   - The host controller applies the LED states to any connected keyboard
   - This creates a complete synchronization of keyboard status

This bidirectional communication demonstrates the power of the Hurricane USB Interface Manager to coordinate complex USB operations across multiple controllers.

## Key Features

### 1. HID Proxy Functionality
The example implements a complete USB HID proxy that can pass through both mouse movements and keyboard keystrokes from a physical USB device to the host computer, with minimal latency.

### 2. Keyboard LED Handling
The implementation synchronizes keyboard LED states between the host computer and any connected keyboard devices, ensuring a consistent user experience.

### 3. Dynamic Interface Configuration
The example demonstrates runtime interface configuration to adapt to connected devices and hosts, allowing for flexible USB device presentation.

### 4. Robust Device Enumeration
The host controller implements comprehensive USB device enumeration to identify and interact with a wide range of HID devices.

### 5. Clean Abstraction Layers
The code maintains clear separation between application logic, device-specific functionality, and host operations for better maintainability.

### 6. Efficient Resource Management
The implementation leverages the dual physical USB controllers of the LPC55S69, avoiding the need for time-multiplexing resources.

## Differences from the RT1060 Implementation

The LPC55S69 and RT1060 examples both demonstrate dual USB functionality, but there are some significant architectural differences:

1. **Controller Architecture**
   - LPC55S69: Two physically separate USB controllers (IP3511 FS for device, EHCI HS for host)
   - RT1060: Single physical controller with two logical instances (both HS)

2. **Resource Sharing**
   - LPC55S69: Minimal resource sharing needed due to separate physical controllers
   - RT1060: More complex resource management to handle shared hardware resources

3. **Clock Configuration**
   - LPC55S69: Separate clock sources for each controller (48MHz for USB0, 480MHz for USB1)
   - RT1060: Single PLL source with dividers for both controllers

4. **Performance Characteristics**
   - LPC55S69: USB0 is Full-Speed only (12Mbps), USB1 is High-Speed (480Mbps)
   - RT1060: Both controllers are High-Speed capable (480Mbps)

5. **Synchronization Requirements**
   - LPC55S69: Minimal synchronization needed between controllers
   - RT1060: More complex synchronization for shared resources

6. **Initialization Sequence**
   - LPC55S69: Controllers can be initialized independently
   - RT1060: Careful initialization sequence required to avoid conflicts

These differences result in a simpler implementation on the LPC55S69 for certain aspects, while the RT1060's dual High-Speed capability offers performance advantages in other scenarios.

## Customization Options

This example can be customized in several ways:

### Device Mode Customization
- Modify HID report descriptors to support different input capabilities
- Change the USB VID/PID for specific device identification
- Adjust the HID report generation behavior (mouse patterns, keyboard text)
- Add additional USB interfaces (CDC, Mass Storage, etc.)

### Host Mode Customization
- Extend support for additional USB device classes
- Implement custom HID report parsing
- Add support for multiple simultaneous connected devices
- Customize device filtering and prioritization

### Application Logic Customization
- Implement more sophisticated bidirectional data translation
- Add command processing or configuration options
- Create specialized USB protocol conversion functions
- Implement data logging or analysis features

## Known Limitations

1. **Full-Speed Device Mode**
   - USB0 controller is limited to Full-Speed (12 Mbps) operation
   - This may impact performance for high-bandwidth applications

2. **Single Device Support**
   - The current host implementation handles only one connected device at a time
   - Additional work would be needed to support multiple simultaneous devices

3. **Basic HID Support**
   - Currently only supports standard HID mouse and keyboard
   - Non-standard HID devices might require additional parsing logic

4. **Limited Error Handling**
   - The example focuses on demonstrating functionality rather than robust error handling
   - Production applications would need more comprehensive error recovery

5. **Power Management**
   - The example does not implement advanced power management features
   - In battery-powered applications, additional power optimization would be needed

## Troubleshooting Guide

### USB Device Issues

1. **Device Not Enumerating**
   - Check USB cable connections to USB0 port
   - Verify power supply is adequate
   - Ensure device descriptors are properly configured
   - Check for USB enumeration errors in console output

2. **HID Reports Not Working**
   - Verify endpoint configuration is correct
   - Check that report descriptors match the expected format
   - Ensure IN transfers are properly scheduled

3. **Inconsistent Device Behavior**
   - Reset both USB controllers if synchronization issues occur
   - Check for endpoint stalls or unhandled control requests
   - Verify device state transitions are properly managed

### USB Host Issues

1. **Device Not Detected**
   - Check physical connection to USB1 port
   - Ensure power supply to the port is sufficient
   - Verify host controller initialization is complete
   - Check for enumeration errors in console output

2. **Unable to Process HID Reports**
   - Verify interrupt IN endpoint is correctly configured
   - Check that report parsing matches the device's format
   - Ensure callback functions are properly registered

3. **LED Status Not Synchronized**
   - Verify SET_REPORT requests are correctly formatted
   - Check that the device supports the HID output reports
   - Ensure bidirectional communication paths are working

### System-Level Troubleshooting

1. **Reset or Initialization Failures**
   - Check that proper clock configuration is in place
   - Verify PHY initialization parameters
   - Ensure controller reset sequences are complete before use

2. **Resource Exhaustion**
   - Monitor memory usage for buffer overflows
   - Check for endpoint resource exhaustion
   - Verify interrupt priorities are appropriate

3. **Communication Delays**
   - Adjust polling intervals for more responsive operation
   - Check for missed interrupts or delayed processing
   - Optimize data paths for lower latency

## Further Development

This example provides a solid foundation for building more complex USB applications on the LPC55S69 platform. Potential areas for further development include:

1. **Additional USB Classes**
   - Add CDC for serial communication
   - Implement Mass Storage for file transfer
   - Support Audio class for sound input/output

2. **Enhanced Host Capabilities**
   - Support for multiple simultaneous devices
   - USB hub implementation
   - Class-specific advanced functionality

3. **Security Features**
   - Implement USB filtering for secure applications
   - Add authentication for connected devices
   - Secure data transfer between host and device endpoints

4. **Integration with Other Peripherals**
   - Combine USB functionality with Bluetooth for wireless extensions
   - Use internal storage to cache or modify USB data
   - Add display output for status and configuration

## Related Documentation
- [Hurricane USB Interface Manager API](../../lib/hurricane/core/usb_interface_manager.h)
- [LPC55S69 USB HAL Implementation](../../lib/hurricane/hw/boards/lpc5500/)
- [LPC55S69 Dual HID Example Build Guide](./BUILD_GUIDE.md)