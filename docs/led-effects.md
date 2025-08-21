# FastLED Library Support for Scribe

This implementation adds FastLED library support to the Scribe ESP32-C3 thermal
printer project with a **runtime-configurable LED effects system**.

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

### Enable/Disable LED Support

Edit `src/core/config.h`:

```cpp
// Uncomment the following line to enable LED strip support
#define ENABLE_LEDS
```

### Runtime Configuration

**All LED settings are now runtime-configurable via the web interface at
`/settings.html`.**

The LED Settings section provides:

- **Hardware Configuration**: GPIO pin selection (0-10) and LED count (1-300)
- **Performance Settings**: Brightness (0-255) and refresh rate (10-120 Hz)
- **Effect Settings**: Fine-tuning for all built-in LED effects

### Default Values

Default settings from `src/core/config.h` (used for initial config.json
creation):

```cpp
static const int DEFAULT_LED_PIN = 4;                    // GPIO pin for LED strip data
static const int DEFAULT_LED_COUNT = 30;                 // Number of LEDs in the strip
static const int DEFAULT_LED_BRIGHTNESS = 64;            // 25% brightness to save power
static const int DEFAULT_LED_REFRESH_RATE = 60;          // 60Hz refresh rate
static const int DEFAULT_LED_EFFECT_FADE_SPEED = 5;      // Fade transition speed
static const int DEFAULT_LED_TWINKLE_DENSITY = 8;        // Simultaneous twinkle stars
static const int DEFAULT_LED_CHASE_SPEED = 3;            // Chase effect speed
static const int DEFAULT_LED_MATRIX_DROPS = 5;           // Matrix drops count
```

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

## API Usage

### Basic Operations

```cpp
#ifdef ENABLE_LEDS
// Initialize in setup()
ledEffects.begin();

// Update in loop() - non-blocking
ledEffects.update();

// Cycle-based effects (recommended - all effects now operate on cycles)
ledEffects.startEffectCycles("chase_single", 1, CRGB::Blue);  // Single blue chase from start to end
ledEffects.startEffectCycles("chase_single", 3, CRGB::Red);   // Three red chase sequences
ledEffects.startEffectCycles("rainbow", 2);                  // Two complete rainbow waves
ledEffects.startEffectCycles("pulse", 1, CRGB::Green);       // One complete pulse cycle
ledEffects.startEffectCycles("matrix", 5, CRGB::Green);      // Five matrix drop cycles

// Time-based effects (legacy support - internally converted to cycles)
ledEffects.startEffectDuration("chase_single", 10, CRGB::Blue);  // 10s blue chase
ledEffects.startEffectDuration("rainbow", 30);                   // 30s rainbow wave
ledEffects.startEffectDuration("matrix", 0, CRGB::Green);        // Infinite green matrix

// Control and query
ledEffects.stopEffect();                            // Stop current effect
bool running = ledEffects.isEffectRunning();        // Check if active
String current = ledEffects.getCurrentEffectName(); // Get current effect
int remainingCycles = ledEffects.getRemainingCycles(); // Get remaining cycles
#endif
```

### Runtime Configuration

```cpp
#ifdef ENABLE_LEDS
// Update LED configuration at runtime
updateLedConfiguration(pin, count, brightness, refreshRate,
                      fadeSpeed, twinkleDensity, chaseSpeed, matrixDrops);

// Get current LED configuration
int pin, count, brightness, refreshRate, fadeSpeed, twinkleDensity, chaseSpeed, matrixDrops;
getLedConfiguration(pin, count, brightness, refreshRate,
                    fadeSpeed, twinkleDensity, chaseSpeed, matrixDrops);

// Reinitialize LED system with new settings (automatically called by updateLedConfiguration)
ledEffects.reinitialize(pin, count, brightness, refreshRate,
                        fadeSpeed, twinkleDensity, chaseSpeed, matrixDrops);
#endif
```

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

## Available Effects

All effects operate on a **cycle-based system** where 1 cycle = one complete pattern execution:

1. **"chase_single"**: Single color chases from start to end (1 cycle = one complete traversal)
2. **"rainbow"**: Rainbow wave effect with configurable wave length (1 cycle = one complete wave)
3. **"twinkle"**: Random twinkling stars with fade effect (1 cycle = one complete twinkle sequence)
4. **"chase_multi"**: Multi-color chase supporting up to 3 colors (1 cycle = one complete traversal)
5. **"pulse"**: Brightness pulse across the entire strip (1 cycle = one complete pulse)
6. **"matrix"**: Falling Matrix-style drops with configurable colors (1 cycle = one complete drop sequence)

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

## Dependencies

- **FastLED 3.7.8+**: Added to both main and test environments in
  `platformio.ini`
- **ESP32-C3 compatible**: Uses appropriate GPIO pins and power limiting
- **Memory safe**: Configurable power limits and refresh rate controls

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
