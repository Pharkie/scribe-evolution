# Scribe ESP32-C3 Thermal Printer Project

This is a C++/Arduino-based ESP32-C3 thermal printer project with web interface,
LED effects, and MQTT integration. The device prints messages from various
sources including ChatGPT, MQTT topics, and quick action buttons.

## Specialized Instructions

For detailed, context-specific guidance:

- **Settings UI & Unbidden Ink**: See
  `.github/instructions/settings-ui.instructions.md`
- **JavaScript Frontend**: See
  `.github/instructions/javascript-frontend.instructions.md`
- **Tailwind CSS**: See `.github/instructions/tailwind-css.instructions.md`
- **Web API Handlers**: See
  `.github/instructions/web-api-handlers.instructions.md`
- **Unit Tests**: See `.github/instructions/unit-tests.instructions.md`

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

### **DRY Principle (Don't Repeat Yourself) - CRITICAL**

**Always apply DRY principle to prevent maintenance nightmares:**

1. **Centralized Constants**: Define all shared values (NVS keys, configuration
   names, API endpoints) in single header files

   - ✅ **Example**: `src/core/nvs_keys.h` - All NVS key names defined once,
     used everywhere
   - ❌ **Never**: Duplicate hardcoded strings across files that can get out of
     sync

2. **Single Source of Truth**: Any value used in multiple places must have ONE
   canonical definition

   - Configuration validation rules
   - Error messages and status codes
   - Default values and ranges
   - Database/storage key names

3. **Shared Utilities**: Extract common functionality into reusable utilities

   - JSON response formatting
   - Validation patterns
   - Error handling patterns
   - Logging helpers

4. **Template/Pattern Reuse**: Use consistent patterns across similar components
   - API endpoint structure
   - Configuration loading/saving
   - Event handling patterns

**When Copilot identifies duplicate code or hardcoded values used in multiple
places, immediately suggest DRY refactoring.**

### C++ Guidelines

1. **File Organization**: Use modular architecture - avoid files >800 lines
2. **DRY Implementation**: Use shared headers for constants (nvs_keys.h,
   shared_types.h)
3. **Naming**: Use camelCase for functions, PascalCase for classes
4. **Includes**: Use relative paths, group by system/library/local
5. **Error Handling**: Use proper error codes and logging
6. **Memory**: Minimize heap allocations, prefer stack/static
7. **Async**: Use ESPAsyncWebServer patterns for web handlers

### Web Development

1. **JavaScript**: Use async/await, proper error handling with toast messages
2. **CSS**: Use Tailwind utility classes, responsive design (sm:, md:, lg:, xl:)
3. **HTML**: Semantic markup, accessibility considerations
4. **API**: RESTful endpoints with JSON responses, proper HTTP status codes
5. **Event Handling**: Use modern addEventListener, avoid inline HTML handlers
6. **Module Architecture**: 4-module system (API, UI, LED, Main coordination)

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
7. **JavaScript modernization**: Converting inline handlers to event listeners
8. **Settings UI fixes**: Time range, frequency, and preset prompt issues

### Tasks to Handle Manually

1. **Hardware debugging**: Printer communication issues
2. **WiFi/network setup**: Environment-specific configuration
3. **Security reviews**: Authentication and API token handling
4. **Major architecture changes**: Large refactoring efforts
5. **Performance optimization**: Memory usage and timing-critical code
6. **Complex time logic**: Unbidden ink scheduling edge cases requiring manual
   testing

## Development Best Practices (Lessons Learned)

### DRY Principle Success Stories

1. **NVS Key Constants (August 2025)**: Created `src/core/nvs_keys.h` to
   centralize all NVS key definitions

   - **Problem**: `api_nvs_handlers.cpp` and `config_loader.cpp` had duplicate
     hardcoded NVS key strings that got out of sync
   - **Symptoms**: Diagnostic endpoint showed keys as "missing" when they
     existed with slightly different names
   - **Solution**: Extracted all NVS key names to constants in shared header,
     both files now import same constants
   - **Result**: Impossible for NVS key names to get out of sync again,
     diagnostic accuracy restored
   - **Lesson**: Always use shared constants for any string/value used in
     multiple files

2. **When to Apply DRY Refactoring**:
   - Any hardcoded string that appears in 2+ files
   - Configuration key names, API endpoints, error messages
   - Magic numbers and default values
   - File paths and resource names
   - Always prefer compile-time constants over runtime string matching

### File Management During Refactoring

3. **Clean Up Temporary Files**: Always remove `_backup`, `_new`, `_old`
   suffixes after refactoring

   - These files can cause linker errors and false build failures
   - Use `find . -name "*_backup*" -o -name "*_new*" | xargs rm` to clean up
   - Verify no duplicate class definitions exist after major refactoring

4. **File Renaming Strategy**: When renaming files systematically:

   - Update all includes first: `#include "OldName.h"` → `#include "NewName.h"`
   - Update class names in headers: `class OldClass` → `class NewClass`
   - Update constructor/destructor names: `OldClass::OldClass()` →
     `NewClass::NewClass()`
   - Update all method references: `OldClass::method()` → `NewClass::method()`
   - Test build after each major batch of changes

5. **Git-Aware File Operations**: When suggesting or executing file moves,
   copies or deletions, always use Git-aware commands rather than plain shell
   commands:
   - For moving/renaming files, use `git mv` instead of `mv`
   - For deleting files, use `git rm` instead of `rm`
   - If untracking without deleting, use `git rm --cached`
   - Avoid suggesting plain `mv` or `rm` unless explicitly operating outside Git
   - Ensure that all Git operations are staged properly so that commits reflect
     the intended changes without unnecessary deletion/addition noise

### Configuration Management

6. **Hardcoded Values**: Always check for hardcoded constants that should move
   to `config.h`

   - Search for numeric literals in effect code: `fadeToBlackBy(strip, 64)` →
     use `CHASE_TRAIL_FADE_STEP`
   - Look for magic numbers in timing: `delay(50)` → use named constants
   - Group related constants logically in config.h with clear comments

7. **Memory Buffer Sizing**: When JSON parsing fails with "NoMemory":
   - Increase `largeJsonDocumentSize` from 2048 to 4096+ bytes
   - Account for long API tokens (ChatGPT tokens are ~200+ chars)
   - Update all related buffer sizes in API handlers consistently

### Build System Integration

8. **Preprocessor Directives**: Avoid redefinition warnings:
   - Use build flags (`-DENABLE_LEDS=1`) OR config.h defines, not both
   - Always use `#ifndef MACRO_NAME` guards in headers
   - Clean builds resolve most preprocessor caching issues

### User Experience Patterns

9. **Toast Messages**: Avoid duplicate user feedback:

   - Remove "processing..." toasts for instant operations (LED off, settings
     save)
   - Keep progress indicators only for operations with meaningful duration
   - Server-side operations should trigger feedback internally, not via separate
     JS API calls

10. **LED Effect Architecture**:

- Name effects clearly: `chase_single` vs `chase_multi`, not confusing `chase`
  vs `simple_chase`
- Use consistent parameter patterns: cycles for sequence effects, duration for
  continuous effects

### Large File Management

11. **Proactive Refactoring**: Monitor files approaching 800+ lines:

- Break by functional responsibility, not arbitrary size
- Use modular patterns: base classes, registries, separate concerns
- Test that refactored modules have same external API

### API Design Consistency

12. **Server-Side Logic Preference**:
    - Configuration changes should trigger confirmations server-side, not
      client-side
    - LED effects should be internal responses to successful operations

### Error Handling Philosophy

13. **Fail Fast - No Silent Fallbacks**:
    - Never use default values or fallbacks that mask missing backend data
    - Throw explicit errors when expected configuration or DOM elements are
      missing
    - Let the frontend crash with clear error messages rather than operating
      with wrong assumptions
    - This makes debugging immediate and obvious rather than hiding problems

### JavaScript Frontend Architecture (Critical Patterns)

14. **Event Handling Modernization**:

    - ALWAYS use `addEventListener()` instead of inline HTML handlers
    - Remove all `onclick`, `oninput`, `onchange` attributes from HTML
    - Set up event listeners in `setupEventHandlers()` function
    - Use proper event delegation for dynamic content
    - Preserve all functionality when converting from inline handlers

15. **Modular JavaScript Structure**:
    - **settings-api.js**: HTTP requests and server communication
    - **settings-ui.js**: DOM manipulation, form handling, visual updates
    - **settings-led.js**: LED-specific functionality and demo effects
    - **settings-main.js**: Coordination, initialization, event listener setup
    - Export functions via `window.ModuleName = { ... }` pattern
    - Keep global exports minimal - most handled by event listeners

## Development Notes

- Use `#ifdef ENABLE_LEDS` for LED-related code
- Prefer `String` over `char*` for Arduino compatibility
- Use `DynamicJsonDocument` for JSON parsing with appropriate sizes
- Handle async web requests properly with error responses
- Maintain consistent code formatting and documentation

### JavaScript/Frontend Specific Notes

- Always run `npm run build-js-prod` after JavaScript changes
- Use modern `addEventListener()` instead of inline HTML handlers
- See `.github/instructions/settings-ui.instructions.md` for detailed settings
  UI patterns
- Test all interactive elements after modernizing event handlers
