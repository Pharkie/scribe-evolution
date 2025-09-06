# Code Structure Overview

Current layout and responsibilities for the Scribe ESP32‑C3 Thermal Printer codebase.

## Table of Contents

- [Overview](#overview)
- [Directory Structure](#directory-structure)
- [Web Assets Directory](#web-assets-directory-data)
- [Core Components](#core-components)
- [Web Layer](#web-layer)
- [Content System](#content-system)
- [Hardware Interfaces](#hardware-interfaces)
- [Utilities](#utilities)
- [Build System](#build-system)
- [Key Files](#key-files)
- [Development Guidelines](#development-guidelines)

## Overview

The Scribe Evolution codebase follows a modular architecture with clear separation of
concerns. The code is organized into logical directories that group related
functionality together, making the codebase more maintainable and extensible.

### Architecture Principles

- **Separation of Concerns**: Each directory handles a specific aspect of
  functionality
- **Modularity**: Components can be developed and tested independently
- **Clear Dependencies**: Include paths explicitly show component relationships
- **Scalability**: Easy to add new features without disrupting existing code

## Directory Structure

```
src/
├── main.cpp                 # Arduino entry point and main application loop
├── core/                    # Core system functionality
├── web/                     # Web server and HTTP handling
├── content/                 # Content generation and management
├── hardware/                # Hardware interfaces and drivers
└── utils/                   # Utility functions and helpers

data/                        # Web assets organized by type
├── css/                     # Stylesheets
├── html/                    # HTML templates and pages
├── js/                      # Modular JavaScript files
├── resources/               # Data files and content
└── favicon.ico              # Website icon

docs/                        # Documentation
```

## Web Assets (source → output)

- Source directories
  - `src/css-source/` (Tailwind input)
  - `src/js-source/` (ES modules: api, stores, pages)
- Build output (served by firmware)
  - `data/css/app.css[.gz]`
  - `data/js/*.js[.gz]`
  - `data/*.html.gz`, `data/settings/*.html.gz`, `data/diagnostics/*.html.gz`
  - `data/resources/*` (JSON, text, PEM)

Build commands: `npm run dev` (unminified) and `npm run build` (minified, gzipped).

## Core Components

**Location**: `src/core/`

The core directory contains fundamental system components that other modules
depend on.

### Files:

- **`config.h.example`** & `config.h`: Compile‑time defaults template and local copy (gitignored)
- **`config_loader.{h,cpp}`**: Load/persist `RuntimeConfig` from NVS
- **`config_utils.{h,cpp}`**: Validation, helpers
- **`shared_types.h`**: Common data structures and type definitions
- **`logging.h`** & **`logging.cpp`**: Centralized logging system
- **`network.h`** & **`network.cpp`**: WiFi connection and network management
- **`mqtt_handler.h`** & **`mqtt_handler.cpp`**: MQTT client and message
  handling

### Key Responsibilities:

- System configuration and validation
- Network connectivity (WiFi, mDNS)
- MQTT communication for remote printing
- Logging to multiple outputs (Serial, File, MQTT, BetterStack)
- Shared data structures used across components

### Global Variables:

- `Message currentMessage`: Shared message structure for printing
- Configuration variables: WiFi credentials, MQTT settings, logging preferences

## Web Layer (HTTP + Auth)

**Location**: `src/web/`

The web layer provides the HTTP interface and web-based control panel.

### Files:

- **`web_server.{h,cpp}`**: Main web server setup and routing
- **`web_handlers.h`** & **`web_handlers.cpp`**: Static file serving and basic
  endpoints
- **`api_handlers.h`** & **`api_handlers.cpp`**: API endpoints for content
  generation
- **`validation.{h,cpp}`**: Input validation and rate limiting
- **`auth_middleware.{h,cpp}`**: Session cookie auth, CSRF for POST, public path rules

### Notes

- AP mode (setup): captive portal, setup endpoints are public.
- STA mode: all `/api/*` endpoints require session auth (including `/api/routes` and `/api/timezones`).
- CSRF protection required on POST/PUT/DELETE (client sends `X-CSRF-Token`, cookie provided on index).

## Content System

**Location**: `src/content/`

The content system handles all content generation, from simple text to
AI-powered responses.

### Files:

- **`content_generators.h`** & **`content_generators.cpp`**: Core content
  generation functions
- **`content_handlers.h`** & **`content_handlers.cpp`**: Web endpoint handlers
  for content
- **`unbidden_ink.h`** & **`unbidden_ink.cpp`**: AI-powered automated content
  generation

### Content Types:

- **Riddles**: Random riddle selection with difficulty levels
- **Jokes**: Dad jokes and humor content
- **Quotes**: Inspirational and motivational quotes
- **Quizzes**: Interactive question and answer content
- **Unbidden Ink**: AI-generated personalized content using external APIs

### Unbidden Ink Features:

- **Scheduled Generation**: Automatic content creation within defined time
  windows
- **AI Integration**: External API calls for dynamic content
- **Customizable Prompts**: User-defined content themes and styles
- **Smart Scheduling**: Frequency control and collision avoidance

## Hardware Interfaces

**Location**: `src/hardware/`

Hardware interfaces manage all physical device interactions.

### Files:

- **`printer.h`** & **`printer.cpp`**: Thermal printer control and communication
- **`hardware_buttons.h`** & **`hardware_buttons.cpp`**: Physical button
  handling

### Printer Features:

- **Multi-Protocol Support**: Serial and other communication methods
- **Character Mapping**: ASCII to printer-specific character conversion
- **Print Formatting**: Text wrapping, alignment, and styling
- **Error Handling**: Robust error detection and recovery

### Button System:

- **Configurable GPIO**: Flexible pin assignment
- **Debouncing**: Clean signal processing
- **Multiple Actions**: Short press vs. long press functionality
- **Rate Limiting**: Prevents button spam and accidental triggers

### Configuration:

Button mappings are defined in configuration files with support for:

- Custom GPIO pin assignments
- Debounce timing adjustment
- Action endpoint mapping
- Rate limiting parameters

## Utilities

**Location**: `src/utils/`

Utility functions provide common functionality used across multiple components.

### Files:

- **`time_utils.h`** & **`time_utils.cpp`**: Time and date management
- **`character_mapping.h`** & **`character_mapping.cpp`**: Character set
  conversions
- **`api_client.{h,cpp}`**: HTTP client for external API calls (retry/backoff)

### Key Functions:

- **Time Management**: NTP sync, timezone handling, DST support
- **Character Processing**: ASCII to printer character mapping
- **API Communication**: Secure HTTPS client for external services
- **Date Formatting**: Consistent timestamp generation

## Build System

- PlatformIO (firmware) + Node (assets)
- `platformio.ini` for environments; enhanced upload target builds and uploads FS+FW

## Key Files

### `main.cpp`

The application entry point that orchestrates all components:

**Setup Phase**:

1. Serial communication initialization
2. Hardware stabilization (printer pins)
3. Configuration validation
4. WiFi connection establishment
5. Logging system setup
6. Timezone and NTP synchronization
7. Component initialization (printer, buttons, web server)
8. Service startup (MQTT, mDNS, web server)

**Main Loop**:

- Watchdog timer management
- Network connectivity monitoring
- Hardware button processing
- Web request handling
- MQTT message processing
- Print queue management
- Memory usage monitoring
- Unbidden Ink scheduling

### `config.h.example`

Template configuration file that users copy to `config.h` with their specific
settings:

- WiFi credentials
- MQTT broker details
- API keys and endpoints
- Hardware pin assignments
- Logging preferences

## Development Guidelines

### Adding New Features

1. **Identify the Appropriate Directory**:
   - Core system functionality → `src/core/`
   - Web endpoints or HTTP handling → `src/web/`
   - Content generation → `src/content/`
   - Hardware drivers → `src/hardware/`
   - Shared utilities → `src/utils/`

2. **Follow Naming Conventions**:
   - Use descriptive file names that match functionality
   - Header files (`.h`) contain declarations
   - Source files (`.cpp`) contain implementations
   - Match header and source file names

3. **Include Path Structure**:
   - Use relative paths from `src/` directory
   - Example: `#include "core/config.h"` from main.cpp
   - Example: `#include "../core/logging.h"` from subdirectories

4. **Global Variables**:
   - Declare in appropriate header files with `extern`
   - Define in corresponding source files
   - Document purpose and usage

### Code Organization Best Practices

- **Single Responsibility**: Each file should have a clear, focused purpose
- **Minimal Dependencies**: Avoid circular dependencies between modules
- **Clear Interfaces**: Use well-defined function signatures and data structures
- **Error Handling**: Implement robust error checking and logging
- **Documentation**: Comment complex logic and maintain header documentation

### Web Asset Development

1. **File Organization**:
   - Place CSS files in `/data/css/`
   - Place HTML files in `/data/html/`
   - Place JavaScript files in `/data/js/`
   - Place resources in `/data/resources/`

2. **JavaScript Modularity**:
   - Keep modules focused and under 300 lines when possible
   - Use clear, descriptive function names
   - Avoid global variables; use module-specific scoping
   - Document function purposes with JSDoc comments

3. **HTML Templates**:
   - Use semantic HTML with proper accessibility
   - Include `data-field` attributes for dynamic content
   - Maintain consistent CSS class usage
   - Keep templates reusable and focused

4. **CSS Organization**:
   - Use Tailwind CSS classes for consistent styling
   - Avoid inline styles; use utility classes
   - Maintain responsive design principles

### Testing and Validation

- **Compilation**: Verify clean builds with no warnings
- **Memory Usage**: Monitor RAM and flash usage during development
- **Functionality**: Test all endpoints and hardware interfaces
- **Integration**: Ensure components work together correctly

### Git Workflow

- **Modular Commits**: Separate commits for each logical component
- **Clear Messages**: Describe what was changed and why
- **Branch Strategy**: Use feature branches for major changes
- **Documentation**: Update relevant documentation with code changes

## Troubleshooting

### Common Issues

1. **Include Errors**: Verify relative paths match directory structure
2. **Undefined References**: Ensure proper extern declarations and definitions
3. **Memory Issues**: Check for memory leaks and excessive heap usage
4. **Build Failures**: Verify all dependencies are properly configured

### Debugging Tools

- **Serial Logging**: Use LOG_VERBOSE, LOG_INFO, LOG_ERROR macros
- **Memory Monitoring**: Built-in heap usage reporting
- **Web Diagnostics**: Comprehensive system status via `/status` endpoint
- **MQTT Logging**: Remote log monitoring for deployed devices

---

This documentation should be updated as the codebase evolves to maintain
accuracy and usefulness for developers working on the Scribe Evolution project.
