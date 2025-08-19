# Scribe ESP32-C3 Thermal Printer Project

ESP32-C3 thermal printer with web interface, LED effects, and MQTT integration.

## Specialized Instructions

- **Settings UI**: `.github/instructions/settings-ui.instructions.md`
- **JavaScript**: `.github/instructions/javascript-frontend.instructions.md`
- **Tailwind CSS**: `.github/instructions/tailwind-css.instructions.md`
- **Web API**: `.github/instructions/web-api-handlers.instructions.md`
- **Unit Tests**: `.github/instructions/unit-tests.instructions.md`

## Project Structure

- `src/main.cpp`: Arduino entry point
- `src/core/`: System functionality (config, network, MQTT, logging)
- `src/web/`: Web server and API handlers
- `src/hardware/`: Hardware interfaces (printer, buttons)
- `src/leds/`: FastLED effects (conditional with ENABLE_LEDS)
- `src/content/`: Content generation and Unbidden Ink
- `src/utils/`: Utilities (time, character mapping, JSON)
- `src/js/`: Frontend JavaScript (minified to data/js/)
- `src/css/`: Tailwind CSS (compiled to data/css/)
- `data/`: Web assets served from ESP32 filesystem
- `test/`: Unit tests
- `docs/`: Documentation

## Build System

### Pre-commit Requirements
- `npm run build-css && npm run build-js-prod` 
- Ensure `config.h` exists (copy from `config.h.example`)
- `pio check` for static analysis
- `pio test` for unit tests

### Key Commands
- **Build**: `pio run -e main`
- **Upload**: `pio run --target upload_main -e main`
- **Test**: `pio test -e test`
- **Frontend**: `npm run build` or `npm run watch-css`

## Core Principles

### DRY (Don't Repeat Yourself) - CRITICAL
- Define shared values once in headers (nvs_keys.h, shared_types.h)
- Use constants for strings used in multiple files
- Extract common functionality to utilities
- Maintain single source of truth for configuration, errors, defaults

### Code Standards
- **Files**: Keep under 800 lines, use modular architecture
- **Naming**: camelCase functions, PascalCase classes
- **Memory**: Minimize heap allocations, prefer stack/static
- **Async**: Use ESPAsyncWebServer patterns
- **JavaScript**: Use addEventListener(), avoid inline handlers
- **CSS**: Tailwind utilities, mobile-first responsive design

## Hardware & Features

### ESP32-C3 Configuration
- Target: `esp32-c3-devkitc-02`
- GPIO 0-10 for LED strips
- 4MB flash with LittleFS
- Thermal printer via UART

### Key Features
- **Web Interface**: Responsive design, dark mode, toast feedback
- **LED Effects**: 6 effects (simple_chase, rainbow, twinkle, chase, pulse, matrix)
- **Content Generation**: ChatGPT integration, quick actions, Unbidden Ink scheduling
- **MQTT Integration**: Message distribution, multi-device networking

## Testing & Development

### Task Suitability
**Good for Copilot:**
- Bug fixes with clear reproduction steps
- UI improvements and responsive design fixes
- Feature additions with well-defined requirements
- Code cleanup and DRY refactoring
- Unit test additions

**Handle Manually:**
- Hardware debugging and printer communication
- WiFi/network environment setup
- Major architecture changes
- Performance optimization
- Complex scheduling logic requiring manual testing

## Development Notes

- Use `#ifdef ENABLE_LEDS` for LED-related code
- Prefer `String` over `char*` for Arduino compatibility
- Use `DynamicJsonDocument` with appropriate buffer sizes
- Handle async web requests with proper error responses
- Always run `npm run build-js-prod` after JavaScript changes
- Use modern `addEventListener()` instead of inline HTML handlers
