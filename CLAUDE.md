# CLAUDE.md

<system_context>
Multi-board ESP32 thermal printer with web interface. Alpine.js + Tailwind CSS frontend, C++ backend.
Supports ESP32-C3-mini, ESP32-S3-mini, and custom PCB variants.
Memory-constrained embedded system with dual configuration layers.
</system_context>

<critical_notes>

- Always run `npm run build` after ANY frontend changes
- Never edit `/data/` directly - edit `/src/data/` instead
- Never hardcode GPIO pins - use board-specific defaults from `boards/board_config.h`
- Board type auto-detected at compile-time from PlatformIO build flags
- Fail fast principle - no fallback values or defensive arrays
- Always use "Scribe Evolution" (never just "Scribe") in all documentation
  </critical_notes>

<file_map>
Specialized CLAUDE.md files provide focused guidance:

- `/src/core/CLAUDE.md` → System fundamentals (config, network, MQTT, logging)
- `/src/config/boards/CLAUDE.md` → Multi-board GPIO configuration system
- `/src/web/CLAUDE.md` → HTTP server, API handlers, validation
- `/src/content/CLAUDE.md` → Content generation (jokes, riddles, AI integration)
- `/src/hardware/CLAUDE.md` → Printer interface, physical buttons
- `/src/leds/CLAUDE.md` → LED effects system (conditional with ENABLE_LEDS)
- `/src/utils/CLAUDE.md` → Utilities (time, character mapping, API client)
- `/src/data/CLAUDE.md` → Frontend source files (Alpine.js + Tailwind)
- `/data/CLAUDE.md` → Auto-generated warning (DO NOT EDIT)
- `/mock-server/CLAUDE.md` → Local development server
  </file_map>

<paved_path>
Setup and Development Workflow:

```bash
# Initial setup
cp src/config/device_config.h.example src/config/device_config.h
npm install

# Development cycle (choose your board)
npm run build

# ESP32-C3-mini
pio run --target upload -e esp32c3-4mb-dev && pio run --target uploadfs -e esp32c3-4mb-dev

# ESP32-S3-mini
pio run --target upload -e esp32s3-8mb-dev && pio run --target uploadfs -e esp32s3-8mb-dev

# ESP32-S3-custom-PCB
pio run --target upload -e esp32s3-8mb-custom-pcb-dev && pio run --target uploadfs -e esp32s3-8mb-custom-pcb-dev

# Testing
pio test -e esp32c3-4mb-test
pio device monitor
```

</paved_path>

<patterns>
Dual Configuration System:
1. Compile-time defaults in `src/core/config.h`
2. Runtime configuration in NVS (Non-Volatile Storage)

Configuration Naming Convention:

- `default` prefix = NVS-backed settings that can be overridden via web interface (e.g., `defaultWifiSSID`, `defaultMqttEnabled`)
- No prefix = Compile-time only settings (e.g., `logLevel`, `betterStackToken`, `jokeAPI`)
- NEVER use `default` prefix for non-NVS constants

Frontend Stack:

- Alpine.js 3.14.9 (reactive framework)
- Tailwind CSS 4.1.12 (utility-first styling; minimal custom CSS for fonts and a keyframes animation in `src/css-source`)
- esbuild 0.25.9 (bundler)

Backend Stack:

- ESP32 variants: C3-mini, S3-mini, S3-custom-PCB (with eFuse protection)
- PlatformIO build system with multi-board support
- Express.js 5.1.0 (mock server)
- Unity test framework

Board Configuration System:

- Compile-time board selection via PlatformIO build flags
- Board-specific GPIO mappings (C3: 0-21, S3: 0-48)
- Runtime board mismatch detection with automatic GPIO reset
- eFuse support for custom PCB (printer/LED power control)
- See `/src/config/boards/CLAUDE.md` for details

Content Generation Architecture:

- Content generators always return structured data: {header, body}
- Local printing: formats as "header\n\nbody"
- MQTT sending: sends {header, body, sender} JSON
- MQTT receiving: constructs "header from sender\n\nbody"
- No backwards compatibility - fail fast principle
  </patterns>

<workflow>
Code Quality Process:
1. Make changes to source files
2. Run `npm run build` (if frontend changes)
3. Test with hardware or mock server
4. Commit with descriptive message
</workflow>

<fatal_implications>

- Edit `/data/` directly = Changes lost on build
- Hardcode values = Maintenance nightmare
- Custom reactivity instead of Alpine = You're doing it wrong
- Missing GPIO validation = Hardware damage
- Deploy MQTT without security review = Network vulnerabilities
