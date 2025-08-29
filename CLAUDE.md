# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Essential Build Commands

### Setup and Configuration
```bash
# Initial setup - copy and customize configuration
cp src/core/config.h.example src/core/config.h
# Edit config.h with your WiFi credentials, MQTT settings, etc.

# Install frontend dependencies
npm install
```

### Development Workflow
```bash
# Build all frontend assets (CSS + JS)
npm run build

# Build and upload everything to ESP32
pio run --target upload -e esp32c3-dev && pio run --target uploadfs -e esp32c3-dev

# Development: watch for frontend changes
npm run watch

# Run tests
pio test -e esp32c3-test

# Run single test file  
pio test -e esp32c3-test -f test_config_validation

# Build firmware only (no upload)
pio run -e esp32c3-dev

# Upload firmware only
pio run --target upload_main -e esp32c3-dev

# Monitor serial output
pio device monitor
```

### Frontend Build Details
- **Production builds**: `npm run build` (minified CSS + compressed JS)
- **Development builds**: `npm run watch` (unminified, file watching)
- **Individual components**: `npm run build-css-settings`, `npm run build-js-index`, etc.

## Architecture Overview

### Project Structure
- **`src/core/`**: System fundamentals (config, network, MQTT, logging)
- **`src/web/`**: HTTP server, API handlers, validation
- **`src/content/`**: Content generation (jokes, riddles, AI integration)
- **`src/hardware/`**: Printer interface, physical buttons
- **`src/leds/`**: LED effects system (conditional with ENABLE_LEDS)
- **`src/utils/`**: Utilities (time, character mapping, API client)
- **`data/`**: Web assets served from ESP32 filesystem
- **`test/`**: Unity test suite

### Dual Configuration System
The project uses a two-layer configuration architecture:

1. **Compile-time defaults** in `src/core/config.h`:
   - Hardware GPIO assignments
   - System constants and limits  
   - Default prompts and autoprompts for Unbidden Ink
   - Network timeouts and buffer sizes

2. **Runtime configuration** in NVS (Non-Volatile Storage):
   - User settings (WiFi, MQTT, device owner)
   - Persistent state (memo content, button actions)
   - LED configurations and effects

**Critical**: All defaults should be defined once in `config.h` and referenced throughout the codebase. Never hardcode values in multiple places.

### Frontend Architecture
- **Alpine.js**: Reactive data binding and state management
- **Tailwind CSS**: Utility-first responsive styling  
- **Modular JavaScript**: Page-specific stores (`*-alpine-store.js`) and API layers (`*-api.js`)
- **Template-based**: HTML templates with dynamic content injection

### Key Global Structures
- **`Message currentMessage`**: Shared print job structure (defined in `shared_types.h`)
- **`RuntimeConfig`**: Active configuration loaded from NVS + defaults
- **GPIO validation functions**: `isValidGPIO()`, `isSafeGPIO()`, `getGPIODescription()` in `config.h`

### Web API Design
- **Modular handlers**: Separate files for config, LEDs, system, content, etc.
- **Consistent validation**: All inputs validated through `validation.cpp`
- **Rate limiting**: Built-in protection against abuse
- **JSON responses**: Standardized error/success format

### Hardware Abstraction
- **ESP32-C3 specific**: GPIO validation, pin safety checks
- **Thermal printer**: UART-based communication with character mapping
- **Hardware buttons**: Configurable GPIO with debouncing and rate limiting
- **LED system**: FastLED-based effects with cycle management (optional)

## Critical Development Principles

### DRY (Don't Repeat Yourself) - CRITICAL
- Define constants once in headers (`nvs_keys.h`, `config.h`)
- Single source of truth for all configuration values
- Extract common functionality to utilities
- **No backwards compatibility** - exactly one way to do things

### Frontend Error Handling
- Frontend should NOT mask backend errors with fallback values
- Display clear error messages when backend data is missing
- Use `console.error()` for debugging, visible errors for users
- Never hardcode backend data in frontend as fallback

### Memory Management
- Minimize heap allocations on ESP32-C3
- Use `DynamicJsonDocument` with appropriate buffer sizes
- Monitor memory usage in main loop
- Prefer stack/static allocation where possible

### GPIO Safety
- Always use `isValidGPIO()` and `isSafeGPIO()` functions from `config.h`
- Never hardcode GPIO pin lists - use the validation system
- ESP32-C3 has limited safe pins (avoid strapping pins, UART pins, etc.)

### Code Organization
- Files should stay under 800 lines
- Use modular architecture with clear separation of concerns
- Include paths: relative from `src/` (e.g., `#include "core/config.h"`)
- Follow naming: camelCase functions, PascalCase classes

### Testing
- Unit tests use Unity framework in `test/` directory
- Test individual components, not integration scenarios
- Use test doubles for hardware-dependent code
- Run tests before committing: `pio test -e test`

## Special Features

### Unbidden Ink (AI Content)
- Scheduled AI-generated content using ChatGPT API
- Autoprompts defined in `config.h`: Creative, Wisdom, Humor, DoctorWho
- Default prompt is Creative (set in `defaultUnbiddenInkPrompt`)
- Time-window scheduling with collision avoidance

### LED Effects System
- Conditional compilation with `#ifdef ENABLE_LEDS`
- Cycle-based effects: ChaseSingle, ChaseMulti, Rainbow, Twinkle, Pulse, Matrix
- Effect registry pattern for extensibility
- Hardware validation for LED GPIO pins

### MQTT Integration
- Multi-printer networking capability
- Remote printing from anywhere via MQTT topics
- Device discovery and status broadcasting
- Topic structure: `scribe/{deviceOwner}/print`

### Mock Server
- Local development server in `mock-server/` directory
- Frontend testing without ESP32 hardware rebuilds
- Simulates all API endpoints with realistic responses

## Common Pitfalls

### Build Issues
- Always run `npm run build` after frontend changes
- Ensure `config.h` exists before building firmware
- Check for missing include paths when adding new files
- Verify ESP32-C3 board selection in PlatformIO

### Configuration Problems  
- Don't hardcode values that should come from `config.h`
- Use appropriate JSON buffer sizes for API responses
- Validate GPIO pins using safety functions
- Ensure NVS namespace consistency ("scribe-app")

### Frontend Development
- Use `addEventListener()` instead of inline handlers
- Keep Alpine stores focused and under 500 lines
- Always handle missing backend data with clear errors
- Build production assets before committing

### Memory Issues
- Monitor heap usage during development
- Use static/stack allocation where possible
- Be careful with String concatenation in loops
- Check JSON document buffer sizes for large responses

This codebase emphasizes modularity, safety, and maintainability. Follow the established patterns and always prioritize the user experience through clear error handling and consistent interfaces.
- Adhere to the principle of failing fast
- Always run npm run build after every CSS or JS change