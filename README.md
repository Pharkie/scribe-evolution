# Scribe Evolution

Your networked ESP32-C3 for thermal printing delights.

![Scribe Thermal Printer](https://github.com/user-attachments/assets/56afd51b-3560-419a-93f4-af315ba2968f)

## Key Features

### Hardware Platform
- **ESP32-C3 support** - Changed from the original project ESP8266 to a more capable ESP32-C3 with hardware serial communication
- **CSN-A4L thermal printer** - Commercial-grade printer with reliable performance

### Network & Remote Capabilities  
- **MQTT remote printing** - Network multiple Scribe printers, print from anywhere
- **mDNS integration** - Access via http://scribe.local or IP address

### Content & Automation
- **Quick fun buttons** - Jokes, quotes, riddles, quiz questions etc
- **Apple Shortcuts support** - Print from anywhere via a HTTP-to-MQTT bridge

### Developer Experience
- **Comprehensive logging** - Multiple output destinations (Serial, file, MQTT, BetterStack)
- **Modular codebase** - Well-organized, documented, and testable architecture  
- **Configuration centralization** - Settings centralised to `src/config.h` then stored to NVS which survive firmware updates, with many user configurable on the front-end.
- **Modern tooling** - PlatformIO, npm build system, automated testing. Not a monolithic Arduino .ino.

## Quick Start

### 1. Hardware Setup
Build your Scribe printer following the [Hardware Guide](docs/hardware.md). You'll need:
- ESP32-C3 development board
- CSN-A4L thermal printer  
- 3D printed enclosure ([download files](https://makerworld.com/en/models/1577165-project-scribe))
- 5V power supply (2.4A+ recommended)

### 2. Software Configuration
```bash
# Create your configuration file
cp src/config.h.example src/config.h

# Edit src/config.h with your settings:
# - WiFi credentials
# - MQTT broker (optional)
# - Timezone preferences
```

### 3. Build and Deploy
```bash
# Install dependencies
npm install

# Build and upload everything
pio run --target upload_all
```

### 4. Access Your Printer
- **Local**: http://scribe-[devicename].local or use IP address from boot message
- **Remote**: Configure MQTT for printing from anywhere

> **Need help?** See the [Build Instructions](docs/build-instructions.md) for detailed setup steps.

## 📚 Documentation

### Getting Started
- **[Build Instructions](docs/build-instructions.md)** - Complete setup guide: PlatformIO, npm, dependencies
- **[Hardware Guide](docs/hardware.md)** - BOM, wiring, assembly, and 3D printing
- **[Microcontroller Firmware](docs/microcontroller-firmware.md)** - Development environment and code architecture

### Integration & Automation  
- **[MQTT Integration](docs/mqtt-integration.md)** - Multi-printer networking and remote control
- **[Apple Shortcuts](docs/apple-shortcuts.md)** - Print from iOS using HTTP-to-MQTT bridges
- **[n8n Integration](docs/n8n-integration.md)** - Workflow automation and webhook processing
- **[Pipedream Integration](docs/pipedream-integration.md)** - Serverless HTTP-to-MQTT bridge

### System Administration
- **[Logging System](docs/logging-system.md)** - Multi-destination logging and monitoring  
- **[Configuration System](docs/configuration-system.md)** - Dual-layer config architecture
- **[Troubleshooting](docs/troubleshooting.md)** - Common issues, diagnostics, and solutions

### Development & Customization
- **[Code Structure](docs/code-structure.md)** - Modular architecture and development guidelines
- **[LED Configuration](docs/LED_CONFIGURATION.md)** & **[Effects](docs/LED_EFFECTS.md)** - Optional LED system
- **[Testing Guide](docs/testing.md)** - Unit testing and validation procedures

### Advanced Topics
- **[HTML Partials Linting](docs/html-partials-linting.md)** - Web interface development
- **[Settings Modular Refactoring](docs/settings-modular-refactoring.md)** - Architecture improvements
- **[Refactoring Summary](docs/refactoring-summary.md)** & **[Release Notes](docs/release.md)** - Project evolution

## Credits and Acknowledgments

### Riddles Collection

The riddles feature uses a collection of riddles curated by **Nikhil Mohite**
from the [riddles-api](https://github.com/nkilm/riddles-api) project. This
collection is provided under the MIT License.

- **Original Repository:** https://github.com/nkilm/riddles-api
- **Author:** Nikhil Mohite
- **License:** MIT License

I thank Nikhil for making this wonderful collection of riddles available to the
open source community.

### Original Project

Credit to **UrbanCircles** for the original Project Scribe concept, 3D
model, and initial codebase that inspired Scribe Evolution.

## Disclaimer

I've done my best to document everything accurately - however, there might be
mistakes. If you see them, or opportunities to improve, please open an issue.  
This is an open-source project given for free, with no warranties or guarantees.
It assumes a level of proficiency with electronics, assemblies, engineering,
etc. Do your own due diligence - it's your responsibility. Stay safe. Stay
productive. Work with what you have. Make the world a better place.

## License

This project is licensed under the **Creative Commons
Attribution-NonCommercial-ShareAlike 4.0 International License**.

### What this means:

✅ **You are free to:**

- **Share** — copy and redistribute the material in any medium or format
- **Adapt** — remix, transform, and build upon the material

⚠️ **Under the following terms:**

- **Attribution** — You must give appropriate credit to Adam Knowles, provide a
  link to the license, and indicate if changes were made
- **NonCommercial** — You may not use the material for commercial purposes
  without explicit written permission
- **ShareAlike** — If you remix, transform, or build upon the material, you must
  distribute your contributions under the same license

### Commercial Use

**Commercial use is prohibited without a licensing agreement.** If you wish to
use this project commercially (including but not limited to selling products
based on this design, using it in commercial environments, or incorporating it
into commercial software), please contact the author.

### Full License

You can view the full license text at:
https://creativecommons.org/licenses/by-nc-sa/4.0/

### Copyright (extended code)

© 2025 Adam Knowles. All rights reserved.

**Original Project:** This project, with a new foundation for the codebase, was inspired by the original Project Scribe by UrbanCircles.