# Scribe ESP32-C3 Thermal Printer Project

This is a C++/Arduino-based ESP32-C3 thermal printer project with web interface,
LED effects, and MQTT integration. The device prints messages from various
sources including ChatGPT, MQTT topics, and quick action buttons.

## Project Structure

### Core Directories

- `src/main.cpp`: Main Arduino entry point
- `src/core/`: Core system functionality (config, network, MQTT, logging)
- `src/web/`: Web server and API handlers (modularized architecture)
- `src/hardware/`: Hardware interfaces (printer, buttons)
- `src/leds/`: FastLED effects system (conditional compilation with ENABLE_LEDS)
- `src/content/`: Content generation and Unbidden Ink features
- `src/utils/`: Utility functions (time, character mapping, JSON helpers)
- `src/js/`: Frontend JavaScript (source files, get minified to data/js/)
- `src/css/`: Frontend CSS using Tailwind (source files, get compiled to
  data/css/)
- `data/`: Web assets served from ESP32 filesystem (HTML, compiled CSS/JS)
- `test/`: Unit tests using PlatformIO testing framework
- `docs/`: Project documentation and configuration guides

## Build System

### Required Before Each Commit

- Run `npm run build-css && npm run build-js-prod` to compile frontend assets
- Ensure `config.h` exists (copy from `config.h.example` and customize)
- Run `pio check` for static analysis
- Run `pio test` for unit tests

### Development Flow

- **Build**: `pio run -e main`
- **Upload & Monitor**: `pio run --target upload_main -e main` (custom Python
  upload script)
- **Test**: `pio test -e test`
- **Clean**: `pio run -t clean`
- **Frontend Build**: `npm run build` (builds both CSS and JS)
- **Frontend Watch**: `npm run watch-css` for development

### Upload Process (upload_main.py)

1. Setup Python environment and dependencies
2. Build Tailwind CSS from src/css/ → data/css/
3. Minify JavaScript from src/js/ → data/js/
4. Upload filesystem (LittleFS)
5. Upload firmware and start monitor
6. Check for files >800 lines needing refactor

## Code Standards

### C++ Guidelines

1. **File Organization**: Use modular architecture - avoid files >800 lines
2. **Naming**: Use camelCase for functions, PascalCase for classes
3. **Includes**: Use relative paths, group by system/library/local
4. **Error Handling**: Use proper error codes and logging
5. **Memory**: Minimize heap allocations, prefer stack/static
6. **Async**: Use ESPAsyncWebServer patterns for web handlers

### Web Development

1. **JavaScript**: Use async/await, proper error handling with toast messages
2. **CSS**: Use Tailwind utility classes, responsive design (sm:, md:, lg:, xl:)
3. **HTML**: Semantic markup, accessibility considerations
4. **API**: RESTful endpoints with JSON responses, proper HTTP status codes

### Configuration System

- Main config in `src/core/config.h` (not committed, use config.h.example)
- Runtime configuration via `/api/config` endpoints
- Validation in `src/web/validation.cpp` with comprehensive error handling
- Support for WiFi, MQTT, ChatGPT API, LED settings, Unbidden Ink

## Hardware Configuration

### ESP32-C3 Specifics

- Target board: `esp32-c3-devkitc-02`
- GPIO pins 0-10 recommended for LED strips
- Built-in USB CDC for serial communication
- 4MB flash with LittleFS filesystem

### Printer Integration

- Thermal printer via UART (configurable pins)
- Character mapping for special characters
- Print queue management with async processing

### LED Effects (Optional)

- FastLED library integration
- 6 effects: simple_chase, rainbow, twinkle, chase, pulse, matrix
- Conditional compilation with `ENABLE_LEDS=1`
- Interactive testing via web interface

## Key Features & Patterns

### Web Interface

1. **Responsive Design**: Mobile-first with proper breakpoints
2. **Dark Mode**: Full theme support with system preference detection
3. **Toast Messages**: User feedback for all actions
4. **Settings Management**: Comprehensive configuration interface
5. **LED Testing**: Interactive effect triggers with colorful buttons

### API Architecture

1. **Modular Handlers**: Separate files for different endpoint groups
2. **JSON Responses**: Consistent error/success format
3. **Validation**: Input validation with detailed error messages
4. **Rate Limiting**: Protection against spam requests

### Content Generation

1. **ChatGPT Integration**: Direct API calls with token management
2. **Quick Actions**: Predefined content types (jokes, riddles, news, etc.)
3. **Unbidden Ink**: Scheduled autonomous content generation
4. **MQTT Publishing**: Content distribution to other devices

## Testing Strategy

### Unit Tests

- Test core functionality in isolation
- Mock hardware interfaces for CI
- Validate configuration parsing and validation
- Test character mapping and time utilities

### Integration Tests

- API endpoint testing with mock requests
- LED effect system validation
- Configuration save/load cycles
- MQTT message handling

## Common Tasks for Copilot

### Good Tasks for Automation

1. **Bug fixes**: Specific error conditions with clear reproduction steps
2. **UI improvements**: Layout adjustments, responsive design fixes
3. **Feature additions**: Well-defined new endpoints or effects
4. **Code cleanup**: Removing redundant code, improving error handling
5. **Documentation**: API documentation, configuration guides
6. **Testing**: Adding unit tests for existing functionality

### Tasks to Handle Manually

1. **Hardware debugging**: Printer communication issues
2. **WiFi/network setup**: Environment-specific configuration
3. **Security reviews**: Authentication and API token handling
4. **Major architecture changes**: Large refactoring efforts
5. **Performance optimization**: Memory usage and timing-critical code

## Development Notes

- Use `#ifdef ENABLE_LEDS` for LED-related code
- Prefer `String` over `char*` for Arduino compatibility
- Use `DynamicJsonDocument` for JSON parsing with appropriate sizes
- Handle async web requests properly with error responses
- Maintain consistent code formatting and documentation
