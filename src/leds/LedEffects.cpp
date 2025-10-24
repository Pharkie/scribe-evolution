/**
 * @file LedEffects.cpp
 * @brief Thread-safe LED effects manager using modular effect system
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "LedEffects.h"

#if ENABLE_LEDS

#include <core/logging.h>
#include <core/LogManager.h>
#include <config/config.h>
#include <core/config_loader.h>
#include "effects/EffectRegistry.h"
#include <utils/color_utils.h>
#include <esp_task_wdt.h>

// Global LED array (non-static so it can be accessed from main.cpp for testing)
#define MAX_LEDS 300
CRGB staticLEDs[MAX_LEDS];

// ============================================================================
// LedEffects Singleton Implementation
// ============================================================================

LedEffects& LedEffects::getInstance()
{
    static LedEffects instance;
    return instance;
}

LedEffects::LedEffects() : mutex(nullptr),
                           leds(nullptr),
                           ledCount(0),
                           ledPin(0),
                           ledBrightness(0),
                           ledRefreshRate(0),
                           ledUpdateInterval(0),
                           ledEffectFadeSpeed(0),
                           ledTwinkleDensity(0),
                           ledChaseSingleSpeed(0),
                           ledChaseMultiSpeed(0),
                           ledMatrixDrops(0),
                           effectActive(false),
                           currentEffectName(""),
                           effectStartTime(0),
                           lastUpdate(0),
                           targetCycles(1),
                           completedCycles(0),
                           effectColor1(CRGB::Blue),
                           effectColor2(CRGB::Black),
                           effectColor3(CRGB::Black),
                           effectStep(0),
                           effectDirection(1),
                           effectPhase(0.0f),
                           currentEffect(nullptr),
                           effectRegistry(nullptr),
                           finalFadeActive(false),
                           finalFadeStart(0),
                           finalFadeBase(nullptr),
                           initialized(false)
{
    // Mutex creation moved to begin() to ensure FreeRTOS is fully initialized
}

LedEffects::~LedEffects()
{
    if (currentEffect)
    {
        delete currentEffect;
        currentEffect = nullptr;
    }

    if (effectRegistry)
    {
        delete effectRegistry;
        effectRegistry = nullptr;
    }

    // LED array is now static (no deallocation needed)

    if (mutex != nullptr)
    {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
}

bool LedEffects::begin()
{
    if (initialized)
    {
        LOG_VERBOSE("LEDS", "LedEffects already initialized");
        return true;
    }

    // Create LED mutex for multi-core protection (must be done in begin(), not constructor)
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr)
    {
        LOG_ERROR("LEDS", "Failed to create LED mutex!");
        return false;
    }

    initialized = true;

    ManagerLock lock(mutex, "LEDS", 2000);
    if (!lock.isLocked())
    {
        LOG_ERROR("LEDS", "Failed to acquire LED mutex in begin()");
        return false;
    }

    // Load configuration
    const RuntimeConfig &config = getRuntimeConfig();

    // Enable LED eFuse if present (custom PCB only) - one-time init
#if BOARD_HAS_EFUSES
    pinMode(BOARD_EFUSE_LED_PIN, OUTPUT);
    digitalWrite(BOARD_EFUSE_LED_PIN, HIGH);
    LOG_VERBOSE("LEDS", "LED strip eFuse enabled (GPIO %d)", BOARD_EFUSE_LED_PIN);
#endif

    // Call internal reinitialize (mutex already held)
    return reinitializeInternal(config.ledPin, config.ledCount, config.ledBrightness,
                                config.ledRefreshRate, config.ledEffects);
}

bool LedEffects::reinitialize(int pin, int count, int brightness, int refreshRate,
                              const LedEffectsConfig &effectsConfig)
{
    if (!initialized)
    {
        LOG_ERROR("LEDS", "LedEffects not initialized - call begin() first!");
        return false;
    }

    ManagerLock lock(mutex, "LEDS", 2000);
    if (!lock.isLocked())
    {
        LOG_ERROR("LEDS", "Failed to acquire LED mutex in reinitialize()");
        return false;
    }

    return reinitializeInternal(pin, count, brightness, refreshRate, effectsConfig);
}

bool LedEffects::reinitializeInternal(int pin, int count, int brightness, int refreshRate,
                                      const LedEffectsConfig &effectsConfig)
{
    // IMPORTANT: Mutex must already be held by caller!

    // Stop current effect (internal version - mutex already held)
    stopEffectInternal();

    // Store configuration
    ledPin = pin;
    ledCount = count;
    ledBrightness = brightness;
    ledRefreshRate = refreshRate;
    ledUpdateInterval = 1000 / refreshRate; // Convert Hz to milliseconds

    // Validate parameters
    if (ledCount <= 0 || ledPin < 0)
    {
        LOG_ERROR("LEDS", "Invalid LED configuration: count=%d, pin=%d", ledCount, ledPin);
        return false;
    }

    // Use static LED array (avoids ESP32-C3 RMT crash with heap allocation)
    if (ledCount > MAX_LEDS)
    {
        LOG_ERROR("LEDS", "LED count %d exceeds maximum %d", ledCount, MAX_LEDS);
        return false;
    }
    leds = staticLEDs;  // Point to static global array

    // Initialize FastLED
    // Validate GPIO pin using configuration system
    if (!isSafeGPIO(ledPin))
    {
        LOG_ERROR("LEDS", "GPIO %d cannot be used for LEDs: %s", ledPin, getGPIODescription(ledPin));
        return false;
    }

    // Clear LED buffer before initializing
    // NOTE: DO NOT call FastLED.clearData() before addLeds() - causes ESP32-C3 crash!
    // FastLED.clearData();  // ← REMOVED: This corrupts ESP32-C3 RMT state
    FastLED.clear();      // Clear all LEDs to off state

    LOG_VERBOSE("LEDS", "Initializing FastLED on GPIO %d (Board: %s)", ledPin, BOARD_NAME);

    // FastLED requires GPIO pin as compile-time template parameter
    // Use macro to reduce repetition
    #define FASTLED_INIT_CASE(pin) \
        case pin: FastLED.addLeds<WS2812B, pin, GRB>(leds, ledCount); break;

    bool initSuccess = false;
    switch (ledPin)
    {
    // Common GPIO pins (0-10, 20-21) - available on all ESP32 boards
    FASTLED_INIT_CASE(0)
    FASTLED_INIT_CASE(1)
    FASTLED_INIT_CASE(2)
    FASTLED_INIT_CASE(3)
    FASTLED_INIT_CASE(4)
    FASTLED_INIT_CASE(5)
    FASTLED_INIT_CASE(6)
    FASTLED_INIT_CASE(7)
    FASTLED_INIT_CASE(8)
    FASTLED_INIT_CASE(9)
    FASTLED_INIT_CASE(10)
    FASTLED_INIT_CASE(20)
    FASTLED_INIT_CASE(21)

    // ESP32-S3 additional GPIO pins - only compile for boards with GPIO > 21
    #if BOARD_MAX_GPIO > 21
    FASTLED_INIT_CASE(11)
    FASTLED_INIT_CASE(12)
    FASTLED_INIT_CASE(13)
    FASTLED_INIT_CASE(14)
    FASTLED_INIT_CASE(15)
    FASTLED_INIT_CASE(16)
    FASTLED_INIT_CASE(17)
    FASTLED_INIT_CASE(18)
    FASTLED_INIT_CASE(33)
    FASTLED_INIT_CASE(34)
    FASTLED_INIT_CASE(35)
    FASTLED_INIT_CASE(36)
    FASTLED_INIT_CASE(37)
    FASTLED_INIT_CASE(38)
    FASTLED_INIT_CASE(39)
    FASTLED_INIT_CASE(40)
    FASTLED_INIT_CASE(41)
    FASTLED_INIT_CASE(42)
    FASTLED_INIT_CASE(43)
    FASTLED_INIT_CASE(44)
    FASTLED_INIT_CASE(47)
    FASTLED_INIT_CASE(48)
    #endif

    default:
        LOG_ERROR("LEDS", "GPIO %d not implemented in FastLED switch (this is a code bug)", ledPin);
        return false;
    }

    initSuccess = true;
    #undef FASTLED_INIT_CASE

    if (!initSuccess)
    {
        LOG_ERROR("LEDS", "FastLED initialization failed for GPIO %d", ledPin);
        return false;
    }

    FastLED.setBrightness(ledBrightness);
    FastLED.clear();
    FastLED.show();

    // Create or update effect registry
    if (effectRegistry)
    {
        effectRegistry->updateConfig(effectsConfig);
    }
    else
    {
        effectRegistry = new EffectRegistry(effectsConfig);
    }

    LOG_VERBOSE("LEDS", "LED system initialized: pin=%d, count=%d, brightness=%d, refresh=%dHz",
                ledPin, ledCount, ledBrightness, ledRefreshRate);

    return true;
}

void LedEffects::update()
{
    if (!initialized)
    {
        // Not initialized yet - silently return (this is called every loop iteration)
        return;
    }

    // Try to acquire LED mutex (non-blocking to prevent loop delays)
    ManagerLock lock(mutex, "LEDS", 100);
    if (!lock.isLocked())
    {
        return;
    }

    // Validate effect state before proceeding
    if (!effectActive || !currentEffect || !leds)
    {
        return;
    }

    // Validate LED count
    if (ledCount <= 0 || ledCount > 300)
    {
        LOG_ERROR("LEDS", "Corrupted ledCount: %d - stopping effect", ledCount);
        stopEffectInternal();
        return;
    }

    unsigned long now = millis();

    // Check if it's time to update
    if (now - lastUpdate < ledUpdateInterval)
    {
        return;
    }

    lastUpdate = now;

    // Update the current effect
    bool shouldContinue = currentEffect->update(leds, ledCount, effectStep, effectDirection,
                                                effectPhase, effectColor1, effectColor2, effectColor3,
                                                completedCycles);

    // Check if cycle-based effect is complete (when target cycles > 0)
    if (targetCycles > 0 && completedCycles >= targetCycles)
    {
        LOG_NOTICE("LEDS", "Effect '%s' complete after %d cycles", currentEffectName.c_str(), completedCycles);
        stopEffectInternal();
        return;
    }

    // Feed watchdog before potentially slow FastLED.show() operation
    esp_task_wdt_reset();

    // Show the updated LEDs
    FastLED.show();
}

bool LedEffects::startEffectCycles(const String &effectName, int cycles,
                                   CRGB color1, CRGB color2, CRGB color3)
{
    if (!initialized)
    {
        LOG_ERROR("LEDS", "LedEffects not initialized - call begin() first!");
        return false;
    }

    ManagerLock lock(mutex, "LEDS", 1000);
    if (!lock.isLocked())
    {
        LOG_ERROR("LEDS", "Failed to acquire LED mutex in startEffectCycles()");
        return false;
    }

    if (!effectRegistry || !effectRegistry->isValidEffect(effectName))
    {
        LOG_WARNING("LEDS", "Unknown effect name: %s", effectName.c_str());
        return false;
    }

    // Stop current effect (internal - mutex already held)
    stopEffectInternal();

    // Create new effect
    currentEffect = effectRegistry->createEffect(effectName);
    if (!currentEffect)
    {
        return false;
    }

    // Initialize effect if needed
    currentEffect->initialize(ledCount);

    // Set effect parameters
    currentEffectName = effectName;
    effectColor1 = color1;
    effectColor2 = color2;
    effectColor3 = color3;
    effectStartTime = millis();
    effectActive = true;
    targetCycles = cycles;
    completedCycles = 0;
    finalFadeActive = false;
    finalFadeStart = 0;
    if (finalFadeBase)
    {
        delete[] finalFadeBase;
        finalFadeBase = nullptr;
    }

    // Reset effect state
    effectStep = 0;
    effectDirection = 1;
    effectPhase = 0.0f;

    LOG_NOTICE("LEDS", "Started LED effect: %s (cycles: %d) on GPIO %d with %d LEDs",
                effectName.c_str(), cycles, ledPin, ledCount);

    return true;
}

bool LedEffects::startEffectCyclesAuto(const String &effectName, int cycles)
{
    if (!initialized)
    {
        LOG_ERROR("LEDS", "LedEffects not initialized - call begin() first!");
        return false;
    }

    // Get colors from registry first (no lock needed for read-only operation)
    String h1, h2, h3;
    {
        ManagerLock lock(mutex, "LEDS", 1000);
        if (!lock.isLocked())
        {
            LOG_ERROR("LEDS", "Failed to acquire LED mutex in startEffectCyclesAuto()");
            return false;
        }

        // Use autonomous per-effect default colors from registry
        if (!effectRegistry)
        {
            LOG_WARNING("LEDS", "Effect registry not initialized");
            return false;
        }

        effectRegistry->getDefaultColorsHex(effectName, h1, h2, h3);
    } // Lock released here

    // Convert to CRGB (fallbacks if strings are empty)
    CRGB c1 = CRGB::Blue;
    CRGB c2 = CRGB::Black;
    CRGB c3 = CRGB::Black;

    // Convert only when hex provided; otherwise keep sensible defaults
    if (h1.length() > 0)
    {
        c1 = hexToRgb(h1);
    }
    if (h2.length() > 0)
    {
        c2 = hexToRgb(h2);
    }
    if (h3.length() > 0)
    {
        c3 = hexToRgb(h3);
    }

    // startEffectCycles will acquire its own lock
    return startEffectCycles(effectName, cycles, c1, c2, c3);
}

void LedEffects::stopEffect()
{
    if (!initialized)
    {
        LOG_ERROR("LEDS", "LedEffects not initialized - call begin() first!");
        return;
    }

    ManagerLock lock(mutex, "LEDS", 1000);
    if (!lock.isLocked())
    {
        LOG_ERROR("LEDS", "Failed to acquire LED mutex in stopEffect()");
        return;
    }

    stopEffectInternal();
}

void LedEffects::stopEffectInternal()
{
    // IMPORTANT: Mutex must already be held by caller!

    if (currentEffect)
    {
        delete currentEffect;
        currentEffect = nullptr;
    }

    effectActive = false;
    currentEffectName = "";

    // Clear all LEDs - be extra thorough
    if (leds && ledCount > 0)
    {
        // First clear our LED array
        for (int i = 0; i < ledCount; i++)
        {
            leds[i] = CRGB::Black;
        }

        // Show the cleared state
        FastLED.show();

        // Additional safety: clear again and show again to ensure it takes effect
        fill_solid(leds, ledCount, CRGB::Black);
        FastLED.show();
    }
    finalFadeActive = false;
    finalFadeStart = 0;
    if (finalFadeBase)
    {
        delete[] finalFadeBase;
        finalFadeBase = nullptr;
    }
}

bool LedEffects::isEffectRunning() const
{
    if (!initialized)
    {
        return false;
    }

    // Const method can't acquire mutex in typical pattern, but for simple bool read it's atomic enough
    // On ESP32, aligned 32-bit reads are atomic
    return effectActive;
}

String LedEffects::getCurrentEffectName() const
{
    if (!initialized)
    {
        return "";
    }

    // String copy needs protection
    ManagerLock lock(const_cast<SemaphoreHandle_t&>(mutex), "LEDS", 100);
    if (!lock.isLocked())
    {
        return "";
    }
    return currentEffectName;
}

unsigned long LedEffects::getRemainingTime() const
{
    if (!initialized)
    {
        return 0;
    }

    ManagerLock lock(const_cast<SemaphoreHandle_t&>(mutex), "LEDS", 100);
    if (!lock.isLocked())
    {
        return 0;
    }

    if (!effectActive || effectDuration == 0)
    {
        return 0;
    }

    unsigned long elapsed = millis() - effectStartTime;
    if (elapsed >= effectDuration)
    {
        return 0;
    }

    return effectDuration - elapsed;
}

void LedEffects::updateEffectConfig(const LedEffectsConfig &newConfig)
{
    if (!initialized)
    {
        LOG_ERROR("LEDS", "LedEffects not initialized - call begin() first!");
        return;
    }

    ManagerLock lock(mutex, "LEDS", 1000);
    if (!lock.isLocked())
    {
        LOG_ERROR("LEDS", "Failed to acquire LED mutex in updateEffectConfig()");
        return;
    }

    if (effectRegistry)
    {
        effectRegistry->updateConfig(newConfig);
        LOG_VERBOSE("LEDS", "Updated effect configuration for playground use");
    }
}

#endif // ENABLE_LEDS
