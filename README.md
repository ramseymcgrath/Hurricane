# Hurricane

Hurricane is an open-source C library for transparent and managed HID communications over USB.

[![Code Coverage](https://codecov.io/gh/ramseymcgrath/Hurricane/branch/main/graph/badge.svg)](https://codecov.io/gh/ramseymcgrath/Hurricane)

---

It is designed to give developers full control over USB HID behavior, from enumeration to packet-level traffic forwarding and injection.

## Core Features (MVP)

USB Host mode: Enumerate HID devices, parse descriptors, and receive input reports.

USB Device mode: Emulate HID devices with configurable descriptors and dynamic report generation.

HID Report Proxying: Forward host-received input reports directly to the device port.

Input Injection: Insert synthetic input into live HID streams.

Report Parsing: Support for interpreting and modifying HID input reports in real time.

## Project Structure

```
core/         → USB protocol logic and HID handling
hw/           → Board-specific low-level USB drivers
apps/         → Example applications (proxy, injector, analyzer)
include/      → Public headers
docs/         → Project documentation and development notes
tools/        → Helper scripts for descriptor parsing, report generation
Makefile      → Project build system
```

## Getting Started

_Coming soon._

---

Reference platforms are consumer NXP and esp32 MCUs

---

Milestone 1: enumerate a real USB mouse, forward its movement data through a USB device interface.

## Roadmap
[ ] Host-side control transfer engine

[ ] Device-side HID mouse emulator

[ ] Descriptor mirroring engine

[ ] Report forwarding pipeline

[ ] External injection API

[ ] Multi-device routing (future)

[ ] Descriptor rewriting (future)

[ ] Custom device passthrough (future)

## Contributing
Pull requests, bug reports, and development discussions are welcome.
This project is in early development — contributions at all layers (USB protocol, HID parsing, board support packages) are encouraged.
