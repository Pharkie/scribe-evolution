# Build Instructions

This document provides detailed instructions for building and deploying the Scribe Evolution Thermal Printer firmware.

## Build System Overview

This project uses a simplified npm build system for web assets and PlatformIO for firmware compilation. The build process consists of two main phases:

1. **Web Assets**: HTML, CSS, and JavaScript bundling with ES6 modules and GZIP compression
2. **Firmware**: ESP32 firmware compilation using PlatformIO

## Supported Devices

Scribe Evolution supports multiple ESP32 variants with different configurations:

- **ESP32-C3** - 4MB flash, no OTA (default)
- **ESP32-S3 Mini** - 8MB flash, OTA-capable
- **Lolin32 Lite** - 4MB flash, no OTA

### Available Build Commands

The build system has been simplified to 3 essential commands:

- **`npm run build`** - Production build (minified CSS + minified JS + GZIP compression)
- **`npm run dev`** - Development build (unminified CSS + unminified JS + GZIP compression)
- **`npm run test`** - Run PlatformIO unit tests

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
cp src/core/config.h.example src/core/config.h
```

Edit `src/core/config.h` with your specific settings:

- WiFi credentials (SSID and password)
- Timezone settings
- mDNS hostname
- MQTT broker settings (if using remote printing)
- Logging configuration
- Character limits and preferences

**Important:** The `config.h` file is ignored by git to keep your credentials safe.

## Web Asset Building

### Quick Start

The web interface uses Tailwind CSS and ES6 modules bundled with esbuild:

```bash
npm install      # Install build dependencies
npm run dev      # Development build (unminified)
npm run build    # Production build (minified + gzipped)
```

### Build Process Details

Each build command performs the following steps automatically:

1. **Copy HTML & Assets** - Copies HTML, PNG, ICO, SVG, and webmanifest files to `/data`
2. **Build CSS** - Compiles Tailwind CSS (minified in production)
3. **Bundle JavaScript** - Uses esbuild to bundle ES6 modules into IIFE format for ESP32
4. **GZIP Compression** - Compresses all assets (average 76% reduction)
5. **Clean Uncompressed** - Removes original files, keeping only `.gz` versions

### ES6 Module Architecture

The JavaScript uses modern ES6 modules in development:

- **Source:** `src/js/pages/*.js` - Entry points for each page
- **Stores:** `src/js/stores/*.js` - Alpine.js store factories
- **APIs:** `src/js/api/*.js` - API service modules
- **Output:** `data/js/page-*.js` - Bundled IIFE format for ESP32

**Note:** The ESP32 serves compressed `.gz` files automatically. Uncompressed files are removed to save flash storage space.

## Firmware Building

### Build Targets

The firmware can be built for different environments:

```bash
# Production builds (optimized, minified)
pio run -e c3-4mb-prod               # ESP32-C3 with 4MB flash
pio run -e s3-4mb-prod               # ESP32-S3 with 4MB flash
pio run -e s3-pcb-prod               # ESP32-S3 custom PCB with 8MB flash

# Development builds (debug symbols, verbose logging)
pio run -e c3-4mb-dev                # ESP32-C3 development
pio run -e s3-4mb-dev                # ESP32-S3 development
pio run -e s3-pcb-dev                # ESP32-S3 custom PCB development

# Test environments
pio run -e c3-4mb-test               # Run unit tests
pio run -e s3-printer-test           # Minimal printer test
pio run -e c3-fastled-test           # FastLED crash investigation
```

### Quick Build Commands

```bash
# Build firmware only (default environment)
pio run

# Upload firmware to specific target
pio run -e s3-4mb-prod --target upload

# Monitor serial output
pio device monitor

# Clean build files
pio run --target clean
```

### npm Firmware Shortcuts

```bash
npm run firmware:esp32c3      # Build ESP32-C3 production
npm run firmware:esp32s3      # Build ESP32-S3 production
npm run firmware:lolin32lite  # Build Lolin32 production
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

### Mock Server for Local Development

For rapid frontend development without flashing the ESP32, use the mock server:

```bash
# Start mock server (runs on localhost:3001)
cd mock-server
node mock-api.js
```

The mock server provides:

- **Complete API simulation**: All `/api/*` endpoints with realistic responses
- **Static file serving**: HTML, CSS, JS, and image assets
- **Live data updates**: Dynamic uptime, memory usage, and temperature
- **CORS support**: Cross-origin requests for development tools
- **Hot reload**: Press `r` to restart server, `d` to reload JSON data
- **Real ESP32 behavior**: Proper delays, SSE events, and response formats

**Perfect for:**

- Frontend CSS/JavaScript development
- UI/UX testing and iteration
- API integration testing
- Logo animation and responsive design work

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
# 1. Edit files in src/css-source/ or src/js-source/
# 2. Build assets
npm run build

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

- Check that the ESP32 is connected via USB
- Ensure no other programs are using the serial port
- Try resetting the device before upload
- For ESP32-S3, ensure you're using a data-capable USB cable (not charge-only)

**Asset building failures:**

- Ensure Node.js version is v16 or later
- Delete `node_modules` and run `npm install` again
- Check that source files in `src/css/` and `src/js/` are valid

### Build Configuration

The project uses specific build flags optimized for ESP32:

- C++17 standard
- Optimized size (`-Os`)
- FastLED support (`-DENABLE_LEDS=1` where applicable)
- USB CDC for serial communication (ESP32-C3 and ESP32-S3)
- LittleFS filesystem

#### Device-Specific Configurations

**ESP32-C3:**

- 4MB flash with `partitions_no_ota.csv`
- Bootloader at 0x0000
- Factory app: 2MB

**ESP32-S3 Mini:**

- 8MB flash with `partitions_8mb_ota.csv`
- Bootloader at 0x0000
- Factory app: 2.5MB
- OTA partition: 2.5MB
- LittleFS: ~3MB

**Lolin32 Lite:**

- 4MB flash with `partitions_no_ota.csv`
- Bootloader at 0x1000 (classic ESP32)
- Factory app: 2MB

See `platformio.ini` for complete build configuration details.
