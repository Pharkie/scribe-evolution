# LED Effects Overview (FastLED)

Optional WS2812B LED support with a runtime‑configurable effects manager.

## Features

- **Optional compilation**: Wrapped in `#ifdef ENABLE_LEDS` - zero impact when
  disabled
- **Non-blocking effects**: All effects integrate seamlessly with the main loop
- **Runtime configurable**: All LED settings can be changed via web interface
  without restart
- **Dynamic memory allocation**: Only allocates LED buffer when enabled, sized
  for actual LED count
- **Web interface integration**: Full LED settings panel with validation and Effect Playground
- **6 built-in effects**: Complete set of visual effects for various use cases
- **Cycle-based control**: All effects operate on cycle counting (1 cycle = one complete pattern)
- **Effect Playground**: Interactive testing interface with real-time parameter adjustment
- **Boot effect**: Automatic LED effect on system startup

## Configuration

- Enable at build time (`ENABLE_LEDS`).
- Configure at runtime via Settings → LEDs (pin, count, brightness, refresh, effect presets).

Default values are seeded from `src/core/config.h(.example)` and persisted in NVS.

### Configuration Storage

LED settings are stored in `config.json`:

```json
{
  "leds": {
    "pin": 4,
    "count": 30,
    "brightness": 64,
    "refreshRate": 60,
    "effectFadeSpeed": 5,
    "twinkleDensity": 8,
    "chaseSpeed": 3,
    "matrixDrops": 5
  }
}
```

Changes made via the web interface are **immediately applied** and **persisted**
to the configuration file.

## Usage

Firmware (non‑blocking):

```cpp
#ifdef ENABLE_LEDS
ledEffects.begin();      // in setup()
ledEffects.update();     // in loop()
ledEffects.startEffectCycles("rainbow", 1);
#endif
```

HTTP API (UI uses these):

- `POST /api/leds/test` with JSON `{ "effect": "rainbow", "speed": 50, ... }`
- `POST /api/leds/off`

**Note**: LED configuration changes via the web interface automatically:

1. Validate the new settings
2. Update `config.json`
3. Reload runtime configuration
4. Reinitialize the LED system
5. Apply changes immediately (no restart required)

### Example: Updating LED Settings at Runtime

```cpp
#ifdef ENABLE_LEDS
// Method 1: Via web interface (recommended)
// Navigate to /settings.html -> LED Settings section
// Changes are automatically validated and applied

// Method 2: Programmatically (for custom implementations)
bool success = updateLedConfiguration(
    4,    // GPIO pin 4
    50,   // 50 LEDs
    128,  // 50% brightness
    60,   // 60Hz refresh rate
    5,    // Fade speed
    10,   // Twinkle density
    3,    // Chase speed
    7     // Matrix drops
);

if (success) {
    LOG_NOTICE("APP", "LED configuration updated successfully");
    // LED system automatically reinitialized with new settings
} else {
    LOG_ERROR("APP", "Failed to update LED configuration");
}
#endif
```

## Built‑in Effects

rainbow, pulse, matrix, twinkle, chase_single, chase_multi

### Effect Parameters

Each effect supports the following unified parameters:

- **cycles**: Number of complete patterns to execute (1-10 in playground, unlimited via API)
- **speed**: Animation speed (1-100, higher = faster)
- **intensity**: Effect-specific intensity parameter (1-100)
- **colors**: Array of colors (single color for most effects, 3 colors for chase_multi)

Additional effect-specific parameters:

- **Twinkle**: fadeSpeed, starCount
- **Matrix**: trailLength, colorSpacing, drops
- **Rainbow**: waveLength

## Integration

The LED system is integrated into the main application:

- **Initialization**: Called in `setup()` after hardware buttons initialization
- **Updates**: Called in `loop()` after hardware button checks
- **Logging**: Uses the existing logging system for status messages
- **Conditional compilation**: Zero code size impact when disabled

## Notes

- Non‑blocking update loop; cycle‑based completion and graceful fade for rainbow.
- GPIO safety validated; memory allocated dynamically to match LED count.

## Files

- `src/leds/LedEffects.h`: Class declarations and API with runtime configuration
  support
- `src/leds/LedEffects.cpp`: Effect implementations with dynamic memory
  allocation
- `src/core/config.h`: LED default configuration constants with `DEFAULT_`
  prefixes
- `src/core/config_loader.h`: Runtime configuration structure and LED config
  functions
- `src/core/config_loader.cpp`: LED configuration loading, saving, and
  validation
- `src/main.cpp`: Integration points for initialization, updates, and boot
  effect
- `data/html/settings.html`: Web interface LED Settings section
- `src/js/settings.js`: JavaScript for LED settings form handling
- `src/web/api_handlers.cpp`: LED configuration validation and system
  reinitialization
- `platformio.ini`: FastLED library dependency

## Web Interface

The LED Settings section in the web interface (`/settings.html`) provides:

- **GPIO Pin Selection**: Dropdown for ESP32-C3 compatible pins (0-10)
- **LED Count**: Number of LEDs in the strip (1-300)
- **Brightness Control**: LED brightness from 0-255
- **Refresh Rate**: Update frequency in Hz (10-120)
- **Effect Parameters**: Fine-tuning for all built-in effects
- **Real-time Validation**: Input validation with helpful error messages
- **Live Updates**: Changes apply immediately without restart

### Effect Playground

The **Effect Playground** provides an interactive interface for testing LED effects with real-time parameter adjustment:

#### Control Parameters

- **Effect Selection**: Choose from 6 built-in effects (chase_single, rainbow, twinkle, chase_multi, pulse, matrix)
- **Speed (1-100)**: Effect animation speed
- **Intensity (1-100)**: Effect brightness/density parameter
- **Number of Cycles (1-10)**: How many complete patterns to run during testing

#### Color Controls

- **Primary Color**: Color picker for single-color effects
- **Multi-color Support**: Additional color pickers for chase_multi effect (supports 3 colors)

#### Effect-Specific Parameters

Each effect exposes custom parameters via additional sliders:

- **Twinkle**: Fade Speed, Number of Stars
- **Matrix**: Trail Length, Color Spacing, Number of Drops
- **Rainbow**: Wave Length (affects pattern density)

#### Testing Features

- **🎨 Test Effect**: Runs the selected effect with current parameters
- **💡 Turn Off LEDs**: Immediately stops all effects and turns off LEDs
- **Dynamic Feedback**: Toast notifications show test status (e.g., "Testing chase_single effect for 3 cycles")
- **Live Updates**: Parameter changes are reflected immediately when testing

#### Cycle-Based Operation

All effects in the playground operate on a **cycle-based system**:

- **1 cycle** = One complete pattern execution
- **Chase effects**: 1 cycle = one complete traversal from start to end
- **Rainbow effects**: 1 cycle = one complete wave across the strip
- **Pulse effects**: 1 cycle = one complete brightness pulse
- **Matrix effects**: 1 cycle = one set of drops falling and completing
- **Twinkle effects**: 1 cycle = one complete twinkle sequence

This provides predictable, consistent behavior where users can test exactly the number of complete patterns they want to see.

## Notes

- **Default state**: LEDs disabled (ENABLE_LEDS commented out)
- **Hardware**: Optimized for WS2812B strips with GRB color order
- **Safety**: Power limited to 1A at 5V to prevent hardware damage
- **Performance**: Configurable refresh rate with dynamic update intervals
- **Compatibility**: Works alongside existing status LED system without conflicts
- **Memory**: Dynamic allocation based on actual LED count (more efficient)
- **Configuration**: All settings runtime-configurable via web interface
- **Boot Effect**: Automatic 5-second rainbow effect on successful initialization
- **Cycle-based Operation**: All effects use unified cycle counting for predictable behavior
- **Effect Playground**: Interactive testing interface with real-time parameter adjustment
- **API Consistency**: Both time-based and cycle-based APIs supported for backward compatibility
