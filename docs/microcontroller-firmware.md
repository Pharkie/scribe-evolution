# Microcontroller Firmware Development

This document covers firmware development for the Scribe ESP32-C3 Thermal Printer, including development environment setup, dependencies, configuration management, and development best practices.

## Overview

The Scribe firmware is built using the Arduino framework on the ESP32-C3 platform, providing:
- Web server interface for local and remote printing
- MQTT integration for networked operation
- Comprehensive logging system
- Thermal printer communication
- LED effects support (optional)
- Professional web assets and API endpoints

## Development Environment Setup

### Recommended: VS Code with PlatformIO

**Why PlatformIO?**
- Automatic dependency management
- Integrated debugging capabilities
- Professional IDE features
- Cross-platform compatibility
- Built-in serial monitor and tools

**Setup Steps**:
1. Install [VS Code](https://code.visualstudio.com/)
2. Install the [PlatformIO IDE extension](https://platformio.org/platformio-ide)
3. Open the project folder in VS Code
4. PlatformIO will automatically handle dependencies and board configuration

### Alternative: Arduino IDE

While supported, Arduino IDE has limitations:
- Manual library management required
- Less sophisticated debugging
- Limited project organization

**Setup for Arduino IDE**:
1. Install [Arduino IDE](https://www.arduino.cc/en/software/)
2. Add ESP32 board support
3. Install required libraries manually (see Dependencies section)
4. Configure board settings for ESP32-C3

### System Requirements

**Operating System**: Windows, macOS, or Linux
**RAM**: 4GB minimum, 8GB recommended
**Storage**: 2GB for development tools and libraries
**USB**: Available USB-A or USB-C port for device connection

## Target Hardware

### ESP32-C3 Specifications

**Microcontroller**: ESP32-C3-FH4 (4MB Flash)
**CPU**: 32-bit RISC-V single-core, up to 160 MHz
**Memory**: 400KB SRAM, 4MB Flash
**Connectivity**: WiFi 802.11 b/g/n, Bluetooth 5 LE
**GPIO**: 22 programmable GPIO pins
**Peripherals**: UART, SPI, I2C, PWM, ADC

**Board**: ESP32-C3-DevKitC-02 or compatible
**Features**:
- USB-C connector for programming and power
- Built-in USB-to-Serial converter
- Reset and boot buttons
- RGB LED (GPIO8)
- Breadboard-friendly pinout

### Pin Configuration

**Thermal Printer Communication**:
- GPIO20: TX (to printer RX)
- GPIO21: RX (from printer TX, optional)

**LED Effects** (if enabled):
- GPIO0-10: Recommended for LED strip connections
- Avoid pins used for flash/boot functions

**Hardware Buttons** (optional):
- Any available GPIO with internal pull-up
- Configurable in `config.h`

## Dependencies

The project uses several libraries managed automatically by PlatformIO:

### Core Libraries

**ArduinoLog (v1.1.1+)**
- Professional logging framework
- Multiple output destinations
- Configurable severity levels
- Used for system diagnostics and debugging

**ArduinoJson (v6.21.3+)**
- JSON parsing and generation
- Memory-efficient
- Used for API requests/responses and configuration

**ezTime (v0.8.3+)**
- Advanced timezone handling
- Automatic DST transitions
- NTP synchronization
- Used for accurate timestamps

**PubSubClient (v2.8+)**
- MQTT client implementation
- TLS support for secure communication
- Used for remote printing capabilities

### Web Server Libraries

**ESPAsyncWebServer**
- High-performance async web server
- WebSocket support
- Template processing
- Handles all HTTP requests

**AsyncTCP (v1.1.1+)**
- Asynchronous TCP library
- Required by ESPAsyncWebServer
- Optimized for ESP32 platform

### File System Library

**LittleFS** (built-in)
- Flash file system
- Stores web assets (HTML, CSS, JS)
- Log file storage
- Configuration persistence

### Optional Libraries

**FastLED (v3.7.8+)**
- LED strip control library
- Multiple effect algorithms
- Conditional compilation via `ENABLE_LEDS`
- Used for visual feedback and effects

**NTPClient (v3.2.1+)**
- Network Time Protocol client
- Time synchronization
- Used with ezTime for accurate timestamps

### Dependency Management

**PlatformIO** (recommended):
Dependencies are automatically installed from `platformio.ini`:

```ini
lib_deps = 
    arduino-libraries/NTPClient@^3.2.1
    ropg/ezTime@^0.8.3
    bblanchon/ArduinoJson@^6.21.3
    knolleary/PubSubClient@^2.8
    thijse/ArduinoLog@^1.1.1
    me-no-dev/AsyncTCP@^1.1.1
    https://github.com/me-no-dev/ESPAsyncWebServer.git
    fastled/FastLED@^3.7.8
```

**Arduino IDE**:
Install libraries manually via Library Manager or ZIP import.

## Configuration Management

### Configuration Architecture

The project uses a dual-layer configuration system:

1. **Compile-time config** (`src/config.h`) - Device-specific settings
2. **Runtime config** - User-configurable settings stored in NVS

### Creating Configuration File

**Initial Setup**:
```bash
cp src/config.h.example src/config.h
```

**Git Exclusion**: `config.h` is automatically excluded from version control to protect credentials.

### Configuration Categories

**Network Settings**:
```cpp
static const char* ssid = "YourWiFiNetwork";
static const char* password = "YourWiFiPassword";
static const char* hostname = "scribe";  // mDNS hostname
static const int mdnsPort = 80;          // Web server port
```

**MQTT Settings**:
```cpp
static const char* mqttServer = "your-broker.hivemq.cloud";
static const int mqttPort = 8883;
static const char* mqttUsername = "your-username";
static const char* mqttPassword = "your-password";
```

**Timezone Configuration**:
```cpp
static const char* timezone = "America/New_York";  // Olson timezone
```

**Logging Configuration**:
```cpp
static const int logLevel = LOG_LEVEL_NOTICE;
static const bool logToSerial = true;
static const bool logToFile = false;
static const bool logToMQTT = false;
```

**Hardware Configuration**:
```cpp
static const int printerTX = 20;     // GPIO pin for printer communication
static const int printerRX = 21;     // GPIO pin for printer communication
static const int printerBaud = 9600; // Baud rate for printer
```

**Optional Features**:
```cpp
#define ENABLE_LEDS 1        // Enable LED effects support
#define DISABLE_OTA 1        // Disable over-the-air updates
```

### Runtime Configuration

Settings that can be changed via web interface:
- Content generation preferences
- LED effect settings
- Quick action configurations
- Display preferences

These are stored in ESP32 NVS (Non-Volatile Storage).

## Build Configuration

### PlatformIO Configuration

The `platformio.ini` file defines build settings:

**Build Flags**:
```ini
build_flags = 
    -std=gnu++17              # C++17 standard
    -DCORE_DEBUG_LEVEL=1      # Debug level
    -DFIRMWARE_VERSION=\"1.0.0\"
    -DENABLE_LEDS=1           # Enable LED support
    -DDISABLE_OTA=1           # Disable OTA updates
    -Os                       # Optimize for size
    -ffunction-sections       # Enable dead code elimination
    -fdata-sections
```

**File System Configuration**:
```ini
board_build.filesystem = littlefs
board_build.partitions = partitions_no_ota.csv
```

### Compilation Optimization

**Size Optimization** (`-Os`):
- Optimizes for code size
- Important for ESP32-C3 flash limitations
- Balances performance and storage

**Dead Code Elimination**:
- `-ffunction-sections -fdata-sections`
- Removes unused code from final binary
- Reduces flash usage

**Debug Configuration**:
```ini
# Development build
build_flags = -DCORE_DEBUG_LEVEL=3 -Os

# Production build  
build_flags = -DCORE_DEBUG_LEVEL=1 -Os
```

## Development Workflow

### Initial Setup

1. **Clone repository**
2. **Create configuration**: `cp src/config.h.example src/config.h`
3. **Edit configuration** with your settings
4. **Build project**: `pio run`
5. **Upload firmware**: `pio run --target upload`

### Development Cycle

1. **Make code changes**
2. **Build and test**: `pio run`
3. **Upload to device**: `pio run --target upload`
4. **Monitor output**: `pio device monitor`
5. **Iterate based on logs and behavior**

### Testing

**Unit Tests**:
```bash
pio test -e test           # Run all tests
pio test -e test -f "*validation*"  # Run specific tests
```

**Integration Testing**:
- Test web interface functionality
- Verify MQTT connectivity
- Check printer communication
- Test LED effects (if enabled)

### Advanced Upload

**Enhanced Upload Script**:
```bash
pio run --target upload_all   # Builds assets + uploads filesystem + firmware
```

This script automatically:
1. Builds CSS and JavaScript assets
2. Uploads LittleFS filesystem
3. Uploads firmware
4. Checks for large files needing refactor

## Code Organization

### Directory Structure

```
src/
├── main.cpp                 # Arduino entry point
├── core/                    # Core system functionality
│   ├── config.h            # Configuration constants
│   ├── wifi_manager.cpp    # WiFi connection handling
│   ├── mqtt_handler.cpp    # MQTT communication
│   └── logging.cpp         # Logging system
├── web/                     # Web server and API
│   ├── api_handlers.cpp    # HTTP API endpoints
│   ├── web_server.cpp      # Web server setup
│   └── validation.cpp      # Input validation
├── hardware/               # Hardware interfaces
│   ├── printer.cpp         # Thermal printer communication
│   └── buttons.cpp         # Hardware button support
├── leds/                   # LED effects system
│   ├── led_controller.cpp  # LED management
│   └── led_effects.cpp     # Effect algorithms
├── content/                # Content generation
│   ├── content_generator.cpp
│   └── api_client.cpp      # External API integration
└── utils/                  # Utility functions
    ├── time_utils.cpp      # Time handling
    ├── character_mapping.cpp
    └── json_helpers.cpp    # JSON utilities
```

### Coding Standards

**File Size Limit**: Maximum 800 lines per file
**Naming Convention**: camelCase for functions, PascalCase for classes
**Include Organization**: System, library, then local includes
**Error Handling**: Always check return values and handle errors
**Memory Management**: Prefer stack allocation, minimize heap usage

### Conditional Compilation

**LED Effects**:
```cpp
#ifdef ENABLE_LEDS
    setupLEDs();
    registerLEDEffects();
#endif
```

**Debug Code**:
```cpp
#if CORE_DEBUG_LEVEL >= 3
    Serial.printf("Debug: variable value = %d\n", value);
#endif
```

## Debugging

### Serial Debugging

**Enable Serial Output**:
```cpp
Serial.begin(115200);
LOG_NOTICE("SYSTEM", "Boot started, firmware v%s", FIRMWARE_VERSION);
```

**Monitor Commands**:
```bash
pio device monitor         # PlatformIO
# or
screen /dev/ttyUSB0 115200 # Linux
```

### Common Issues

**Upload Failures**:
- Check USB cable connection
- Ensure no other programs use serial port
- Try different USB port
- Reset device before upload

**WiFi Connection Problems**:
- Verify SSID and password in config.h
- Check WiFi signal strength
- Monitor serial output for connection attempts
- Test with mobile hotspot

**MQTT Connection Issues**:
- Verify broker hostname and credentials
- Check firewall and network access
- Test with MQTT client tools
- Enable verbose logging

**Compilation Errors**:
- Ensure config.h exists and is properly formatted
- Check library versions are compatible
- Clean build directory: `pio run --target clean`
- Update PlatformIO platform: `pio platform update`

### Performance Monitoring

**Memory Usage**:
```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Flash usage: %d%%\n", ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace());
```

**Task Performance**:
```cpp
unsigned long startTime = millis();
performOperation();
Serial.printf("Operation took: %lu ms\n", millis() - startTime);
```

## Best Practices

### Power Management

- Use deep sleep for battery applications
- Implement watchdog timer for reliability
- Monitor power consumption during printing

### Network Resilience  

- Implement WiFi reconnection logic
- Handle MQTT broker disconnections gracefully
- Provide fallback operation when offline

### Flash Memory Management

- Monitor filesystem usage
- Implement log rotation
- Clean up temporary files
- Use compression for large assets

### Security Considerations

- Never hardcode credentials in source code
- Use TLS for MQTT connections
- Validate all user inputs
- Implement rate limiting for API endpoints

### Production Deployment

- Set appropriate log levels
- Disable debug features
- Optimize build for size
- Test extensively before deployment
- Document configuration requirements