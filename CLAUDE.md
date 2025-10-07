# CLAUDE.md

<system_context>
ESP32-C3 thermal printer with web interface. Alpine.js + Tailwind CSS frontend, C++ backend.
Memory-constrained embedded system with dual configuration layers.
</system_context>

<critical_notes>

- Always run `mcp__ide__getDiagnostics` after ANY code changes
- Always run `npm run build` after ANY frontend changes
- Never edit `/data/` directly - edit `/src/data/` instead
- Never hardcode values - use `config.h` and NVS
- Fail fast principle - no fallback values or defensive arrays
- Always use "Scribe Evolution" (never just "Scribe") in all documentation
  </critical_notes>

<file_map>
Specialized CLAUDE.md files provide focused guidance:

- `/src/core/CLAUDE.md` → System fundamentals (config, network, MQTT, logging)
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
cp src/core/config.h.example src/core/config.h
npm install

# Development cycle
npm run build
pio run --target upload -e esp32c3-dev && pio run --target uploadfs -e esp32c3-dev

# Testing
pio test -e esp32c3-test
pio device monitor
```

</paved_path>

<patterns>
Dual Configuration System:
1. Compile-time defaults in `src/core/config.h`
2. Runtime configuration in NVS (Non-Volatile Storage)

Frontend Stack:

- Alpine.js 3.14.9 (reactive framework)
- Tailwind CSS 4.1.12 (utility-first styling; minimal custom CSS for fonts and a keyframes animation in `src/css-source`)
- esbuild 0.25.9 (bundler)

Backend Stack:

- ESP32-C3 with PlatformIO
- Express.js 5.1.0 (mock server)
- Unity test framework

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
2. Run `mcp__ide__getDiagnostics` 
3. Fix any linting/type errors
4. Run `npm run build` (if frontend changes)
5. Test with hardware or mock server
6. Commit with descriptive message
</workflow>

<fatal_implications>

- Skip diagnostics = Broken code ships
- Edit `/data/` directly = Changes lost on build
- Hardcode values = Maintenance nightmare
- Custom reactivity instead of Alpine = You're doing it wrong
- Missing GPIO validation = Hardware damage
- Deploy MQTT without security review = Network vulnerabilities
