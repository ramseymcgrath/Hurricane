# Hurricane

Hurricane is an open-source C library for transparent and managed HID communications over USB.

[![Code Coverage](https://codecov.io/gh/ramseymcgrath/Hurricane/branch/main/graph/badge.svg)](https://codecov.io/gh/ramseymcgrath/Hurricane)

---

It is designed to give developers full control over USB HID behavior, from enumeration to packet-level traffic forwarding and injection.

## Core Features (MVP)

- **USB Host mode**: Enumerate HID devices, parse descriptors, and receive input reports.
- **USB Device mode**: Emulate HID devices with configurable descriptors and dynamic report generation.
- **HID Report Proxying**: Forward host-received input reports directly to the device port.
- **Input Injection**: Insert synthetic input into live HID streams.
- **Report Parsing**: Support for interpreting and modifying HID input reports in real time.

## Supported Hardware

Hurricane currently supports the following hardware platforms:

- Seeed XIAO ESP32-C3
- ESP32-S3-DevKitC-1
- Teensy 4.1 (partial support)

Reference platforms are consumer NXP and ESP32 MCUs.

## PlatformIO Integration

Hurricane is designed to work seamlessly with PlatformIO. Configuration is provided in `platformio.ini`:

```ini
[env:seeed_xiao_esp32c3]
platform = espressif32
board = seeed_xiao_esp32c3
framework = espidf
monitor_speed = 115200
build_flags =
    -Ilib/hurricane
    -Ilib/hurricane/core
    -Ilib/hurricane/usb
    -Ilib/hurricane/hw
    -DPLATFORM_ESP32
```

To build the project with PlatformIO:

```bash
pio run
```

To upload to your device:

```bash
pio run --target upload
```

## Testing

Hurricane includes a comprehensive test suite to ensure code quality and reliability.

### Running Tests

To run the test suite:

```bash
cd test
make run
```

### Test Coverage

To generate a test coverage report:

```bash
cd test
make coverage
```

This will:
1. Build and run the tests with coverage instrumentation
2. Generate a detailed HTML coverage report
3. Place the report in `build/coverage-report/index.html`

## Project Structure

```
core/         → USB protocol logic and HID handling
hw/           → Board-specific low-level USB drivers
apps/         → Example applications (proxy, injector, analyzer)
include/      → Public headers
docs/         → Project documentation and development notes
test/         → Unit and integration tests
tools/        → Helper scripts for descriptor parsing, report generation
Makefile      → Project build system
```

## Getting Started

_Coming soon._

---

Milestone 1: enumerate a real USB mouse, forward its movement data through a USB device interface.

## Roadmap
- [ ] Host-side control transfer engine
- [ ] Device-side HID mouse emulator
- [ ] Descriptor mirroring engine
- [ ] Report forwarding pipeline
- [ ] External injection API
- [ ] Multi-device routing (future)
- [ ] Descriptor rewriting (future)
- [ ] Custom device passthrough (future)

## Contributing
Pull requests, bug reports, and development discussions are welcome.
This project is in early development — contributions at all layers (USB protocol, HID parsing, board support packages) are encouraged.
