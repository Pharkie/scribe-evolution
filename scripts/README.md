Scripts Overview

Structure
- scripts/bin: User-facing entry points you run directly.
- scripts/pio: PlatformIO extra scripts imported by PlatformIO (not run directly).
- scripts/lib: Utilities imported by other scripts.
- scripts/tests: Bench/test utilities.
- scripts/templates: Templates used by release pipeline.

Entry points (scripts/bin)
- build_firmware_release.py: Release build pipeline. Backs up src/core/config.h to src/core/config.h.adam, generates a clean src/core/config.h.example, replaces src/core/config.h with the cleaned version for the build, builds all target envs, then restores the original config.h. Uses scripts/lib/config_cleaner.py.
- check_esp32.py: Quick sanity check that an ESP32‑C3 is connected and ready for upload.
- printer_discovery_sim.py: Local printer discovery/demo simulator (renamed from test_printer_discovery.py).
- optimize_filesystem.py: Minimizes/copies web assets into data/ for LittleFS.

PlatformIO extra scripts (scripts/pio)
- generate_config_example.py: Pre‑build. Keeps src/core/config.h.example fresh by cleaning src/core/config.h via config_cleaner.py.
- build_frontend.py: Pre‑build. Builds CSS/JS before compiling firmware.
- build_upload_monitor.py: Custom task: build frontend → upload filesystem → upload firmware → start serial monitor.

Utilities (scripts/lib)
- config_cleaner.py: Single source of truth for secret detection, replacement, and validation.

Tests (scripts/tests)
- test_esp32c3_gpio.py: Hardware exercise script (bench test, not part of the build).

Recommended flows
- Development build (PlatformIO):
  - Pre-scripts run automatically:
    - scripts/pio/generate_config_example.py → keeps config.h.example up to date
    - scripts/pio/build_frontend.py → builds/minifies web assets

- Firmware release (npm):
  - npm run firmware → scripts/bin/build_firmware_release.py
    1. Back up src/core/config.h → src/core/config.h.adam
    2. Clean secrets and write src/core/config.h.example (with example metadata)
    3. Replace src/core/config.h with the cleaned version for reproducible build
    4. Build firmware for targets and package artifacts
    5. Restore original src/core/config.h from src/core/config.h.adam

Notes
- “One way” for secret cleaning: All scripts import from scripts/lib/config_cleaner.py.
