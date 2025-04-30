# Hurricane

<p align="center">
  <img src="https://codecov.io/gh/ramseymcgrath/Hurricane/branch/main/graph/badge.svg" alt="Code Coverage"/>
  <br>
  <b>Transparent, managed USB HID communications for embedded systems</b>
</p>

---

Hurricane is an open-source C library for transparent and managed HID communications over USB. It gives developers full control over USB HID behavior, from enumeration to packet-level traffic forwarding and injection.

---

## 🚀 Quickstart

### 1. Clone the repository

```bash
git clone https://github.com/ramseymcgrath/Hurricane.git
cd Hurricane
```

### 2. Build and run tests (macOS/Linux)

```bash
cd test
make run
```

### 3. Generate a coverage report

```bash
make coverage
open ../build/coverage-report/index.html  # macOS: open the HTML report
```

### 4. Build for hardware (PlatformIO)

Install [PlatformIO](https://platformio.org/install) if you haven't already.

```bash
pio run
```

To upload to your device:

```bash
pio run --target upload
```

---

## ✨ Core Features

- <b>USB Host mode:</b> Enumerate HID devices, parse descriptors, and receive input reports.
- <b>USB Device mode:</b> Emulate HID devices with configurable descriptors and dynamic report generation.
- <b>HID Report Proxying:</b> Forward host-received input reports directly to the device port.
- <b>Input Injection:</b> Insert synthetic input into live HID streams.
- <b>Report Parsing:</b> Support for interpreting and modifying HID input reports in real time.

## 🛠️ Supported Hardware

| Board                  | Framework | Status         |
|------------------------|-----------|---------------|
| Seeed XIAO ESP32-C3    | ESP-IDF   | ✅ Supported   |
| ESP32-S3-DevKitC-1     | ESP-IDF   | ✅ Supported   |
| Teensy 4.1             | Arduino   | ⚠️ Partial     |

Reference platforms are consumer NXP and ESP32 MCUs.

## ⚙️ PlatformIO Example

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

---

## 🧪 Testing

Hurricane includes a comprehensive test suite to ensure code quality and reliability.

- **Run tests:**
  ```bash
  cd test
  make run
  ```
- **Generate coverage report:**
  ```bash
  make coverage
  open ../build/coverage-report/index.html
  ```

---

## 📁 Project Structure

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

---

## 🗺️ Roadmap
- [ ] Host-side control transfer engine
- [ ] Device-side HID mouse emulator
- [ ] Descriptor mirroring engine
- [ ] Report forwarding pipeline
- [ ] External injection API
- [ ] Multi-device routing (future)
- [ ] Descriptor rewriting (future)
- [ ] Custom device passthrough (future)

---

## 🤝 Contributing
Pull requests, bug reports, and development discussions are welcome.
This project is in early development — contributions at all layers (USB protocol, HID parsing, board support packages) are encouraged.
