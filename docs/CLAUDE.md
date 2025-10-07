# docs/CLAUDE.md

<system_context>
Documentation for Scribe Evolution ESP32-C3 thermal printer.
User guides, integration tutorials, and technical references.
</system_context>

<critical_notes>

- Always use "Scribe Evolution" (never just "Scribe") in all documentation
- Use markdown links for file references: [filename.ts](path/to/filename.ts)
- Include screenshots in `docs/assets/screenshots/` with descriptive paths
- Keep documentation in sync with actual code behavior
- Test all example code and commands before documenting them
  </critical_notes>

<file_map>
Documentation Structure:

- `index.md` → Documentation hub and navigation
- `quick-start.md` → Getting started guide
- `hardware.md` → Hardware assembly and components
- `build-instructions.md` → Build and flash process
- `configuration-system.md` → Config.h and NVS explained
- `mqtt-integration.md` → MQTT setup with HiveMQ
- `pipedream-integration.md` → HTTP→MQTT bridge setup
- `apple-shortcuts.md` → iOS automation integration
- `code-structure.md` → Codebase organization
- `frontend-patterns.md` → Alpine.js and Tailwind patterns
- `led-effects.md` → LED system documentation
- `logging-system.md` → Logging framework guide
- `testing.md` → Unit and integration testing
- `troubleshooting.md` → Common issues and solutions
- `DATA_DRIVEN_CONFIG.md` → Historical config approach
  </file_map>

<patterns>
Documentation Style:

1. **Branding**: Always "Scribe Evolution", never "Scribe"
2. **Code Examples**: Test all commands and payloads before documenting
3. **Screenshots**: Use relative paths like `assets/screenshots/category/file.png`
4. **Links**: Use markdown format `[text](path)` for files, not backticks
5. **Step-by-Step**: Number steps clearly, use bold for UI elements
6. **Troubleshooting**: Include common errors with solutions
7. **Security**: Highlight security best practices prominently

Integration Guides Format:

- Overview with clear value proposition
- Prerequisites section
- Step-by-step setup with screenshots
- Example payloads and API reference
- Troubleshooting section
- Security best practices
  </patterns>

<workflow>
Documentation Updates:
1. Make changes to markdown files
2. Verify all code examples still work
3. Check screenshot paths are valid
4. Ensure "Scribe Evolution" is used consistently
5. Test any commands or API calls documented
6. Commit with descriptive message
</workflow>

<fatal_implications>

- Inconsistent branding = Confusing product identity
- Untested examples = User frustration
- Missing security warnings = Vulnerabilities
- Broken screenshot paths = Poor user experience
- Outdated docs = Support burden and user confusion
