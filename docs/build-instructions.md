# Build Instructions

This document provides detailed instructions for building and deploying the Scribe ESP32-C3 Thermal Printer firmware.

## Build System Overview

This project uses npm for asset building and PlatformIO for firmware compilation. The build process consists of two main phases:

1. **Frontend Assets**: CSS and JavaScript compilation using npm and Tailwind
2. **Firmware**: ESP32-C3 firmware compilation using PlatformIO

## Prerequisites

### Required Software

- **Node.js** (v16 or later) - for CSS/JavaScript asset building
- **PlatformIO** - for firmware compilation
- **Python** (v3.7+) - required by PlatformIO

### Installation

#### VS Code with PlatformIO (Recommended)

1. Install [VS Code](https://code.visualstudio.com/)
2. Install the [PlatformIO IDE extension](https://platformio.org/platformio-ide)
3. Open the project folder in VS Code
4. PlatformIO will automatically handle dependencies and board configuration

#### Alternative: Command Line PlatformIO

```bash
pip install platformio
```

#### Node.js Dependencies

```bash
npm install                # Install build dependencies
```

## Configuration Setup

Before building, you must create your configuration file:

```bash
cp src/config.h.example src/config.h
```

Edit `src/config.h` with your specific settings:
- WiFi credentials (SSID and password)
- Timezone settings
- mDNS hostname
- MQTT broker settings (if using remote printing)
- Logging configuration
- Character limits and preferences

**Important:** The `config.h` file is ignored by git to keep your credentials safe.

## Frontend Asset Building

### CSS & JavaScript Assets

The web interface uses Tailwind CSS and minified JavaScript:

```bash
npm install                # Install build dependencies
npm run build             # Build CSS + JS (with source maps)
npm run build-prod        # Production build (no source maps)
```

### Individual Asset Commands

For development, you can build specific assets:

```bash
# CSS only
npm run build-css
npm run build-css-prod     # Production (no source maps)
npm run watch-css          # Watch mode for development

# JavaScript only  
npm run build-js
npm run build-js-prod      # Production (no source maps)
```

**Note:** The minified files (`data/css/*.min.css` and `data/js/*.min.js`) are committed to the repository for users who don't have Node.js installed. If you modify CSS or JS files, rebuild the assets before committing.

## Firmware Building

### Standard PlatformIO Commands

```bash
# Build firmware only
pio run                    

# Upload firmware only
pio run --target upload    

# Monitor serial output
pio device monitor         

# Clean build files
pio run --target clean
```

### Enhanced Upload Process (Recommended)

Use the enhanced upload script for automatic asset building:

```bash
pio run --target upload_all   # Builds assets + uploads filesystem + firmware
```

This automatically:

1. 🎨 Builds and minifies CSS/JavaScript
2. 📁 Uploads filesystem to ESP32
3. 💾 Uploads firmware to ESP32
4. 📊 Checks for files >800 lines needing refactor

## Development Workflow

### For Code Changes

```bash
# 1. Make your changes to src/ files
# 2. Build and test
pio run
pio run --target upload
pio device monitor

# 3. For release, use enhanced upload
pio run --target upload_all
```

### For Frontend Changes

```bash
# 1. Edit files in src/css/ or src/js/
# 2. Build assets
npm run build-prod

# 3. Upload filesystem and firmware
pio run --target upload_all
```

### Development Tips

- Use `npm run watch-css` during frontend development for automatic rebuilds
- The enhanced upload script performs file size checks - files >800 lines trigger warnings
- Always test your changes before committing
- Run `npm run build-prod` before committing to ensure minified assets are up to date

## Testing

Run the test suite to verify your build:

```bash
pio test -e test           # Run all tests
```

## Troubleshooting

### Common Build Issues

**PlatformIO not found:**
```bash
pip install platformio
# Or use python -m platformio instead of pio
```

**Node.js dependencies missing:**
```bash
npm install
```

**Upload failures:**
- Check that the ESP32-C3 is connected via USB
- Ensure no other programs are using the serial port
- Try resetting the device before upload

**Asset building failures:**
- Ensure Node.js version is v16 or later
- Delete `node_modules` and run `npm install` again
- Check that source files in `src/css/` and `src/js/` are valid

### Build Configuration

The project uses specific build flags optimized for the ESP32-C3:
- C++17 standard
- Optimized size (`-Os`)
- FastLED support (`-DENABLE_LEDS=1`)
- USB CDC for serial communication
- LittleFS filesystem

See `platformio.ini` for complete build configuration details.