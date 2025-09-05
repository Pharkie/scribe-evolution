<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="data/images/ScribeLogoMain-white.svg">
    <source media="(prefers-color-scheme: light)" srcset="data/images/ScribeLogoMain-black.svg">
    <img alt="Scribe Evolution Logo" src="data/images/ScribeLogoMain-black.svg" width="80%" style="display:block;margin:0 auto;" />
  </picture>
</p>

<br>
<br>

<p align="center">
  <img src="docs/assets/Scribe-evolution-demo.gif" alt="Scribe Evolution Demo" width="100%" />
  
</p>

**Print Fast. Share Securely. Poke Friends.**

Scribe Evolution is a small, AI-powered thermal printer that delights you with ink-free, super-fast, tear-and-go printing of quick notes, to-do lists, recipes, news summaries, or trivia. Share any of these instantly with those in your private network.

It’s ready when you are: print from your phone, poke your friends like it’s 1997, or manage your day. It’s your helpful, compact desk assistant.

It has pressable buttons to save your favourite actions for quick access, and lights up as a beautiful object in your home or office.

![Scribe Evolution Thermal Printer](https://github.com/user-attachments/assets/56afd51b-3560-419a-93f4-af315ba2968f)

### 🔑 **Features**

- **Physical Buttons**: Dedicated hardware controls for instant content generation
- **Content Library**: Curated collections of jokes, riddles, quotes, and trivia
- **LED Effects**: Add an LED strip for lighting awesomeness.
- **Apple Shortcuts Ready**: Print anything from your iPhone with a tap

### 🕸️ **Network-First Design**

- **MQTT Integration**: Network multiple printers, print from anywhere in the world
- **mDNS Discovery**: Access via `http://scribe-[devicename].local` - no IP hunting
- **Remote Control**: Print from iOS shortcuts, web services, or custom applications
- **Multi-Device Orchestration**: Coordinate content across multiple Scribe Evolution printers

> ⚠️ **Security Note**: MQTT networking uses a trust-based model suitable for home/private networks. See [MQTT Security](docs/MQTT_SECURITY.md) for limitations and recommendations.

### 🤖 **AI-Powered Content Generation**

- **Quick Fun Actions**: Instant jokes, quotes, riddles, weather reports, and daily news briefings
- **Unbidden Ink**: Scheduled AI-generated content that appears automatically throughout your day

### 🎛️ **Solid Build**

- **Web-Based Interface**: Responsive design works on phones, tablets, and desktops
- **Persistent Settings**: NVS storage preserves all configurations across firmware updates
- **Real-Time Diagnostics**: Monitor system health, memory usage, network status, and hardware
- **Comprehensive Logging**: Multiple output destinations including BetterStack integration

### 🛠️ **Developer Experience**

- **Modern Architecture**: Modular C++ codebase with proper separation of concerns
- **Build Automation**: Integrated npm and PlatformIO workflows
- **Mock Server**: Local development server for frontend testing without ESP32 rebuilds
- **Configuration Management**: Centralized settings with environment-specific overrides
- **Documentation**: Extensive guides covering aspects of the system

## Quick Start

### 🚀 **Ready to Print? Start Here!**

**👉 [Quick Start Guide - Flash Pre-built Firmware →](docs/quick-start.md)**

Get your Scribe printer running in minutes with pre-built firmware. No development environment needed!

---

### 🛠️ **Developer Setup**

Building from source? Follow these steps:

#### 1. Hardware Setup

Build your Scribe Evolution printer following the [Hardware Guide](docs/hardware.md). You'll need:

- ESP32-C3 development board
- CSN-A4L thermal printer
- 3D printed enclosure ([download files](https://makerworld.com/en/models/1577165-project-scribe))
- 5V power supply (2.4A+ recommended)

#### 2. Software Configuration

```bash
# Create your configuration file
cp src/core/config.h.example src/core/config.h

# Edit src/core/config.h with your settings:
# - WiFi credentials
# - MQTT broker (optional)
# - Timezone preferences
```

#### 3. Build and Deploy

```bash
# Install dependencies
npm install

# Build frontend assets and upload everything
npm run build && pio run --target upload -e main && pio run --target uploadfs -e main
```

#### 4. Build Firmware for Distribution

```bash
# Build clean firmware releases for both board types
npm run firmware
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
- **[Pipedream Integration](docs/pipedream-integration.md)** - Serverless HTTP-to-MQTT bridge
- **[Apple Shortcuts](docs/apple-shortcuts.md)** - Print from iOS using HTTP-to-MQTT bridges

### System Administration

- **[Logging System](docs/logging-system.md)** - Multi-destination logging and monitoring
- **[Configuration System](docs/configuration-system.md)** - Dual-layer config architecture
- **[MQTT Security](docs/MQTT_SECURITY.md)** - Security model and limitations for MQTT networking
- **[Troubleshooting](docs/troubleshooting.md)** - Common issues, diagnostics, and solutions

### Development & Customization

- **[Code Structure](docs/code-structure.md)** - Modular architecture and development guidelines
- **[LED Effects](docs/led-effects.md)** - Optional LED system with cycle-based effects
- **[Testing Guide](docs/testing.md)** - Unit testing and validation procedures
- **[Mock Server](mock-server/README.md)** - Local development server for frontend testing

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

### UI Icons

The user interface icons throughout the application are sourced from high-quality
open source icon libraries:

**Heroicons** - Modern SVG icons crafted by the makers of Tailwind CSS

- **Repository:** https://github.com/tailwindlabs/heroicons
- **Authors:** Steve Schoger and Adam Wathan (Refactoring UI Inc.)
- **License:** MIT License
- **Copyright:** © 2020 Refactoring UI Inc.

**Sidekick Icons** - Carefully designed outline icons for modern applications

- **Repository:** https://github.com/ndri/sidekickicons
- **Author:** Andrea Debnar (@ndri)
- **License:** MIT License
- **Copyright:** © 2021 Andrea Debnar

Both icon sets replaced emoji characters throughout the interface to provide
consistent, scalable, and accessible visual elements. All icons maintain
their original MIT License terms.

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

### Copyright

© 2025 Adam Knowles. All rights reserved.

**Original Project:** This project, with a new foundation for the codebase, was inspired by the original Project Scribe by UrbanCircles.
