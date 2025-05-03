# LPC55S69 Dual HID Example - Build and Flash Guide

This guide provides comprehensive instructions for building and flashing the LPC55S69 dual HID example in the Hurricane library. The example demonstrates simultaneous USB host and device operation on the LPC55S69 microcontroller.

## Introduction

The LPC55S69 dual HID example demonstrates the capabilities of the Hurricane USB Interface Manager to manage both USB host and device functionalities simultaneously. This implementation leverages the LPC55S69's dual USB controllers:

- **USB0**: Used in device mode (Full Speed)
- **USB1**: Used in host mode (High Speed EHCI)

The application operates as a composite HID device (mouse and keyboard) while simultaneously being able to detect and interact with connected USB HID devices.

## Prerequisites

### Required Software

- **NXP MCUXpresso SDK** (version 2.8.0 or later)
  - The SDK provides essential drivers and middleware for LPC55S69 development
  - Available from [NXP's website](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools/mcuxpresso-software-development-kit-sdk:MCUXpresso-SDK)

- **ARM GCC Toolchain** (9.x or later)
  - Cross-compiler for ARM Cortex-M33 architecture
  - Available from [ARM's developer website](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) or through package managers

- **CMake** (3.20 or later)
  - Required for the build system
  - Available from [cmake.org](https://cmake.org/download/) or through package managers

- **PyOCD** (0.29.0 or later)
  - Python-based tool for programming and debugging Arm Cortex microcontrollers
  - Can be installed via pip: `pip install pyocd`

- **A Serial Terminal Program**
  - For viewing debug output from the application
  - Examples include PuTTY, TeraTerm, screen, minicom, etc.

### Required Hardware

- **LPC55S69-EVK** development board
  - [NXP LPC55S69-EVK](https://www.nxp.com/design/development-boards/lpcxpresso-boards/lpcxpresso55s69-development-board:LPC55S69-EVK)

- **USB Cables**
  - 1x USB cable for programming/debugging (typically USB-A to Micro-B)
  - 1x USB cable for device mode testing (connecting USB0 to a host computer)
  - Optionally, a USB HID device (mouse or keyboard) for testing host mode

## Development Environment Setup

### Installing the ARM GCC Toolchain

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# Fedora
sudo dnf install arm-none-eabi-gcc-cs
```

#### macOS
```bash
brew install arm-none-eabi-gcc
```

#### Windows
Download and install the toolchain from the [ARM Developer website](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).

### Setting up the NXP MCUXpresso SDK

1. Visit the [NXP MCUXpresso SDK Builder](https://mcuxpresso.nxp.com/en/builder)
2. Select "LPC55S69" as the processor
3. Select the following components:
   - USB (Device, Host, and common components)
   - GPIO
   - Clock
   - UART
4. Download the SDK package
5. Extract the SDK to a known location on your system
   - Note the path, as you'll need it during the build process

### Installing PyOCD

PyOCD can be installed using pip. Make sure you have Python 3.6 or newer installed.

```bash
pip install pyocd
```

For detailed installation instructions, refer to the [PyOCD documentation](https://github.com/pyocd/pyOCD).

After installation, connect your LPC55S69-EVK to your computer and verify PyOCD recognizes it:

```bash
pyocd list
```

You should see your board listed in the output.

## Building the Project

### CMake Configuration

First, create a build directory for the LPC55S69 project:

```bash
# From the Hurricane root directory
mkdir -p build_lpc55s69
cd build_lpc55s69
```

Configure the project with CMake, specifying the path to your NXP SDK:

```bash
cmake .. -DPLATFORM=lpc55s69 -DNXP_SDK_PATH=/path/to/nxp/sdk
cmake .. -DPLATFORM=lpc55s69 -DNXP_SDK_PATH=/Users/ramseymcgrath/code/mcuxpresso-sdk/mcuxsdk 
```

Replace `/path/to/nxp/sdk` with the actual path to your NXP SDK installation.

### Build Commands

After configuration, build the dual HID example:

```bash
# From the build_lpc55s69 directory
make hurricane_lpc55s69_dual_hid
```

This will compile the project and generate the following artifacts in the `build_lpc55s69` directory:

- `hurricane_lpc55s69_dual_hid` - ELF executable
- `hurricane_lpc55s69_dual_hid.bin` - Binary file for flashing

### Build Options

The build system supports several options that can be passed to CMake:

- `-DPLATFORM=lpc55s69`: Specifies the target platform
- `-DNXP_SDK_PATH=/path/to/sdk`: Path to the NXP MCUXpresso SDK
- `-DENABLE_USB_HOST=ON|OFF`: Enable/disable USB host functionality (default: ON)
- `-DENABLE_USB_DEVICE=ON|OFF`: Enable/disable USB device functionality (default: ON)
- `-DENABLE_DUAL_USB=ON|OFF`: Enable/disable dual USB stack support (default: ON)
- `-DCMAKE_BUILD_TYPE=Debug|Release`: Set build type (default: Debug)

Example with additional options:

```bash
cmake .. -DPLATFORM=lpc55s69 -DNXP_SDK_PATH=/path/to/nxp/sdk -DCMAKE_BUILD_TYPE=Release
```

## Flashing to the Target

### Connecting the Hardware

1. Connect the LPC55S69-EVK to your computer using the USB cable to the "Link" port (usually the one near the reset button)
2. Ensure the board is powered on (the power LED should be illuminated)

### USB Port Configuration

The LPC55S69 uses two USB ports in this example:

- **USB0** (Device Mode): Connect this USB port to another computer to test the device functionality
- **USB1** (Host Mode): Connect a USB HID device (like a mouse or keyboard) to test the host functionality

### Flashing with PyOCD

To flash the binary using PyOCD:

```bash
# From the build_lpc55s69 directory
pyocd flash -t LPC55S69 hurricane_lpc55s69_dual_hid.bin
```

You can also use the CMake target that we've configured for easy flashing:

```bash
# From the build_lpc55s69 directory
make flash_lpc55s69_dual_hid
```

Note: The first time you run this command, it might attempt to download the pack for LPC55S69. Allow it to complete to ensure proper flashing.

### Alternative Flashing Methods

#### Using J-Link

If you prefer using SEGGER J-Link:

1. Install [J-Link Software and Documentation Pack](https://www.segger.com/downloads/jlink/)
2. Connect your board to the computer
3. Use the following command to flash:

```bash
JLinkExe -device LPC55S69 -if SWD -speed 4000 -autoconnect 1
# In the J-Link console
loadbin hurricane_lpc55s69_dual_hid.bin 0x0
r
q
```

#### Using MCUXpresso IDE

You can also use NXP's MCUXpresso IDE:

1. Import the project into MCUXpresso IDE
2. Build the project within the IDE
3. Right-click on the project and select "Debug As" > "MCUXpresso IDE LinkServer Debug"

## Testing and Debugging

### Setting up the Serial Console

The dual HID example outputs debug information through UART0 at 115200 baud. To view this output:

1. Connect a USB-to-Serial adapter to the LPC55S69-EVK UART pins:
   - GND to GND
   - RX to P0_30 (UART TX)
   - TX to P0_29 (UART RX)

2. Open a terminal program configured for:
   - 115200 baud rate
   - 8 data bits
   - No parity
   - 1 stop bit
   - No flow control

Example serial terminal commands:

```bash
# Linux/macOS (replace /dev/ttyUSB0 with your device)
screen /dev/ttyUSB0 115200

# Windows (using PuTTY)
# Select COM port and set 115200 baud rate
```

### Testing the Dual HID Functionality

1. **Device Mode Test**:
   - Connect the USB0 port to a computer
   - The board should enumerate as a composite HID device
   - You should observe mouse cursor movements in a circular pattern
   - The keyboard should periodically type predefined text

2. **Host Mode Test**:
   - Connect a USB HID device to the USB1 port
   - Check the serial console for device detection messages
   - The application should report details about the connected device

### Common Issues and Troubleshooting

#### USB Device Not Detected

- Check USB cable connections
- Ensure the correct USB port (USB0) is connected for device mode
- Verify that the USB descriptors are properly configured in the firmware
- Use a USB analyzer tool or system logs to debug enumeration issues

#### USB Host Not Detecting Devices

- Check USB cable connections to the USB1 port
- Ensure the device is powered and functional
- Check if the device is recognized when connected directly to a computer
- Verify host mode initialization in the firmware

#### Flashing Issues

- Ensure proper board connections
- Try resetting the board before flashing
- Check PyOCD installation and updates
- Consider using alternative flashing methods

#### Build Issues

- Verify NXP SDK path and version
- Check compiler installation
- Review CMake configuration options
- Inspect build logs for specific error messages

### USB Debugging Tips

1. **Use USB Enumeration Logs**:
   - On Windows: Device Manager > View > Show hidden devices
   - On macOS: System Report > USB
   - On Linux: `lsusb -v`

2. **Monitor USB Traffic**:
   - Use tools like Wireshark with USBPcap or USB Protocol Analyzer
   - Look for enumeration requests and descriptor exchanges

3. **Check Device Enumeration**:
   - Verify VID/PID in device_config.c matches what's reported by the OS
   - Analyze descriptor structures for compliance issues

4. **Serial Debugging**:
   - Add detailed USB event logging in the code
   - Monitor the serial output during USB operations

## Build Script

To automate the build and flash process, you can create a shell script like the one below.

Create a file named `build_and_flash_lpc55s69.sh` in your project directory:

```bash
#!/bin/bash
# Build and Flash Script for LPC55S69 Dual HID Example

# Configuration
NXP_SDK_PATH="/path/to/your/nxp/sdk"  # Update this path
BUILD_TYPE="Debug"                     # or "Release"
BUILD_DIR="build_lpc55s69"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create build directory
echo -e "${BLUE}Creating build directory...${NC}"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Configure with CMake
echo -e "${BLUE}Configuring project with CMake...${NC}"
cmake .. -DPLATFORM=lpc55s69 \
         -DNXP_SDK_PATH=${NXP_SDK_PATH} \
         -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

# Build the project
echo -e "${BLUE}Building the LPC55S69 dual HID example...${NC}"
make hurricane_lpc55s69_dual_hid

# Check if build was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed! Check the error messages above.${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Flash the binary if requested
if [ "$1" == "flash" ]; then
    echo -e "${YELLOW}Flashing to LPC55S69...${NC}"
    pyocd flash -t LPC55S69 hurricane_lpc55s69_dual_hid.bin
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Flashing failed! Check the error messages above.${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Flashing successful!${NC}"
fi

echo -e "${GREEN}All operations completed successfully!${NC}"
echo -e "To flash manually, run: ${YELLOW}pyocd flash -t LPC55S69 hurricane_lpc55s69_dual_hid.bin${NC}"

cd ..
```

Make the script executable:

```bash
chmod +x build_and_flash_lpc55s69.sh
```

Usage:

```bash
# Build only
./build_and_flash_lpc55s69.sh

# Build and flash
./build_and_flash_lpc55s69.sh flash
```

Remember to update the `NXP_SDK_PATH` variable in the script with your actual SDK path.

## Conclusion

This guide covered the complete process of building, flashing, and testing the LPC55S69 dual HID example. The example demonstrates the Hurricane USB Interface Manager's capabilities to manage simultaneous USB host and device operations on the LPC55S69 microcontroller.

For more details about the example implementation:
- Refer to the source code in `examples/lpc55s69/dual_hid/`
- Review the Hurricane USB Interface Manager API in `lib/hurricane/core/usb_interface_manager.h`
- Check the LPC55S69 USB HAL implementation in `lib/hurricane/hw/boards/lpc5500/`