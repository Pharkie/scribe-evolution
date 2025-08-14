# FastLED Library Support for Scribe

This implementation adds FastLED library support to the Scribe ESP32-C3 thermal printer project with a configurable LED effects system.

## Features

- **Optional compilation**: Wrapped in `#ifdef ENABLE_LEDS` - zero impact when disabled
- **Non-blocking effects**: All effects integrate seamlessly with the main loop
- **Configurable parameters**: All effects support custom colors and durations
- **Memory efficient**: Only allocates LED buffer when enabled
- **6 built-in effects**: Complete set of visual effects for various use cases

## Configuration

### Enable/Disable LED Support

Edit `src/core/config.h`:

```cpp
// Uncomment the following line to enable LED strip support
#define ENABLE_LEDS
```

### Hardware Configuration

Default settings in `src/core/config.h`:

```cpp
static const int LED_PIN = 4;        // GPIO pin for LED strip data
static const int LED_COUNT = 30;     // Number of LEDs in the strip
// LED strip type: WS2812B with GRB color order (configured in LedEffects.cpp)
```

### Performance Settings

```cpp
static const int LED_BRIGHTNESS = 64;              // 25% brightness to save power
static const int LED_REFRESH_RATE = 60;            // 60Hz refresh rate
static const int LED_EFFECT_FADE_SPEED = 5;        // Fade transition speed
static const int LED_TWINKLE_DENSITY = 8;          // Simultaneous twinkle stars
static const int LED_CHASE_SPEED = 3;              // Chase effect speed
static const int LED_MATRIX_DROPS = 5;             // Matrix drops count
```

## API Usage

### Basic Operations

```cpp
#ifdef ENABLE_LEDS
// Initialize in setup()
ledEffects.begin();

// Update in loop() - non-blocking
ledEffects.update();

// Start effects with custom parameters
ledEffects.startEffect("chase", 10, CRGB::Blue);    // 10s blue chase
ledEffects.startEffect("rainbow", 30);              // 30s rainbow wave
ledEffects.startEffect("matrix", 0, CRGB::Green);   // Infinite green matrix

// Control and query
ledEffects.stopEffect();                            // Stop current effect
bool running = ledEffects.isEffectRunning();        // Check if active
String current = ledEffects.getCurrentEffectName(); // Get current effect
unsigned long timeLeft = ledEffects.getRemainingTime(); // Get remaining time
#endif
```

## Available Effects

1. **"simple_chase"**: Color moves start to end with off phase
2. **"rainbow"**: Configurable palette-based rainbow effect  
3. **"twinkle"**: Random groups of 3 LEDs with fade
4. **"chase"**: Continuous chase without off phase
5. **"pulse"**: Brightness pulse across strip
6. **"matrix"**: Falling Matrix-style effect with configurable colors

## Integration

The LED system is integrated into the main application:

- **Initialization**: Called in `setup()` after hardware buttons initialization
- **Updates**: Called in `loop()` after hardware button checks
- **Logging**: Uses the existing logging system for status messages
- **Conditional compilation**: Zero code size impact when disabled

## Dependencies

- **FastLED 3.7.8+**: Added to both main and test environments in `platformio.ini`
- **ESP32-C3 compatible**: Uses appropriate GPIO pins and power limiting
- **Memory safe**: Configurable power limits and refresh rate controls

## Files

- `src/leds/LedEffects.h`: Class declarations and API
- `src/leds/LedEffects.cpp`: Effect implementations and manager logic
- `src/core/config.h`: LED configuration constants and enable flag
- `src/main.cpp`: Integration points for initialization and updates
- `platformio.ini`: FastLED library dependency

## Notes

- Default state: LEDs disabled (ENABLE_LEDS commented out)
- Hardware: Optimized for WS2812B strips with GRB color order
- Safety: Power limited to 1A at 5V to prevent hardware damage
- Performance: 60Hz refresh rate with configurable update intervals
- Compatibility: Works alongside existing status LED system without conflicts