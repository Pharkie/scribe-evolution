# Project Scribe

A networked ESP32-C3 thermal printer for tangible logging of life's meaningful moments, thoughts, and memories. Print from anywhere via MQTT or locally through an elegant web interface.

![Scribe Thermal Printer](https://github.com/user-attachments/assets/56afd51b-3560-419a-93f4-af315ba2968f)

## Key Features

### Enhanced Hardware & Platform
- **ESP32-C3 support** - Upgraded from ESP8266 to ESP32-C3 with hardware serial communication
- **Professional 3D design** - Elegant flowing curves inspired by Dalí and Gaudí
- **Robust construction** - 3D printed enclosure with integrated cable management
- **CSN-A4L thermal printer** - Commercial-grade printer with reliable performance

### Network & Remote Capabilities  
- **MQTT remote printing** - Network multiple Scribe printers, print from anywhere
- **mDNS integration** - Access via http://scribe.local or IP address
- **Secure TLS connections** - Encrypted MQTT communication via cloud brokers
- **Multi-printer web interface** - Control local and remote printers from single UI

### Content & Automation
- **Quick action buttons** - Jokes, quotes, riddles, quiz questions, and test prints
- **API integrations** - icanhazdadjoke.com, ZenQuotes.io, The Trivia API
- **Apple Shortcuts support** - Print from anywhere via HTTP-to-MQTT bridges
- **Webhook integrations** - IFTTT, Zapier, n8n, and custom automation support

### Developer Experience
- **Comprehensive logging** - Multiple output destinations (Serial, file, MQTT, BetterStack)
- **Modular codebase** - Well-organized, documented, and testable architecture  
- **Configuration centralization** - All settings in `src/config.h`
- **Professional tooling** - PlatformIO, npm build system, automated testing

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
- **Local**: http://scribe.local or use IP address from boot message
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

## Core Features

### Hardware & Design
- **Commercial-grade thermal printing** - Clean, permanent records with no ink or mess
- **Low power operation** - Runs on USB power (~0.5W idle, supports standard 57.5mm thermal paper)
- **Elegant 3D printed design** - Organic curves inspired by Dalí and Gaudí ([3D files available](https://makerworld.com/en/models/1577165-project-scribe))

### Connectivity & Control
- **Local web interface** - Minimalist, distraction-free printing with instant feedback
- **MQTT networking** - Print remotely from anywhere, support multiple networked printers
- **External integrations** - Apple Shortcuts, IFTTT, webhooks, and automation platforms

### Content & Entertainment
- **Quick action buttons** - Instant jokes, quotes, riddles, trivia questions, and printer tests
- **API integrations** - Real-time content from icanhazdadjoke.com, ZenQuotes.io, The Trivia API
- **Local content** - 545+ curated riddles stored locally for offline entertainment

## Usage Examples

**Personal logging**: Print daily thoughts, achievements, or memorable moments  
**Smart home integration**: Automated notifications, weather alerts, calendar events  
**Family messaging**: Send messages between rooms using multiple networked printers  
**Developer tool**: Print API responses, log messages, or debugging information  
**Creative projects**: Generate random content for writing prompts or creative inspiration

> **Ready to build?** Start with the [Hardware Guide](docs/hardware.md) and [Build Instructions](docs/build-instructions.md).

## Credits and Acknowledgments

### Riddles Collection

The riddles feature uses a collection of riddles curated by **Nikhil Mohite**
from the [riddles-api](https://github.com/nkilm/riddles-api) project. This
collection is provided under the MIT License.

- **Original Repository:** https://github.com/nkilm/riddles-api
- **Author:** Nikhil Mohite
- **License:** MIT License

We thank Nikhil for making this wonderful collection of riddles available to the
open source community.

### Original Project

All credit to **UrbanCircles** for the original Project Scribe concept, 3D
model, and initial codebase that made this ESP32-C3 thermal printer possible.

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
into commercial software), please contact the authors.

### Full License

You can view the full license text at:
https://creativecommons.org/licenses/by-nc-sa/4.0/

### Copyright (extended code)

© 2025 Adam Knowles. All rights reserved.

**Original Project:** Based on the original Project Scribe by UrbanCircles, used
with permission and adapted under the same Creative Commons license terms.