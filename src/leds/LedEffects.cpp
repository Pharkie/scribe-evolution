/**
 * @file LedEffects.cpp
 * @brief Implementation of LED effects manager for WS2812B LED strips
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "LedEffects.h"

#ifdef ENABLE_LEDS

#include "../core/logging.h"
#include "../core/config_loader.h"

// Global instance
LedEffects ledEffects;

LedEffects::LedEffects() : 
    leds(nullptr),
    ledCount(0),
    ledPin(0),
    ledBrightness(0),
    ledRefreshRate(0),
    ledUpdateInterval(0),
    ledEffectFadeSpeed(0),
    ledTwinkleDensity(0),
    ledChaseSpeed(0),
    ledMatrixDrops(0),
    effectActive(false),
    currentEffectName(""),
    effectStartTime(0),
    effectDuration(0),
    lastUpdate(0),
    effectColor1(CRGB::Blue),
    effectColor2(CRGB::Black),
    effectColor3(CRGB::Black),
    effectStep(0),
    effectDirection(1),
    effectPhase(0.0f),
    twinkleStars(nullptr),
    matrixDrops(nullptr)
{
    // Configuration will be loaded in begin()
}

LedEffects::~LedEffects()
{
    deallocateArrays();
}

void LedEffects::allocateArrays()
{
    deallocateArrays(); // Clean up existing allocations
    
    if (ledCount > 0) {
        leds = new CRGB[ledCount];
    }
    
    if (ledTwinkleDensity > 0) {
        twinkleStars = new TwinkleState[ledTwinkleDensity];
        for (int i = 0; i < ledTwinkleDensity; i++) {
            twinkleStars[i].active = false;
            twinkleStars[i].position = 0;
            twinkleStars[i].brightness = 0;
            twinkleStars[i].fadeDirection = 1;
        }
    }
    
    if (ledMatrixDrops > 0) {
        matrixDrops = new MatrixDrop[ledMatrixDrops];
        for (int i = 0; i < ledMatrixDrops; i++) {
            matrixDrops[i].active = false;
            matrixDrops[i].position = 0;
            matrixDrops[i].length = 0;
            matrixDrops[i].speed = 1;
        }
    }
}

void LedEffects::deallocateArrays()
{
    if (leds) {
        delete[] leds;
        leds = nullptr;
    }
    
    if (twinkleStars) {
        delete[] twinkleStars;
        twinkleStars = nullptr;
    }
    
    if (matrixDrops) {
        delete[] matrixDrops;
        matrixDrops = nullptr;
    }
}

bool LedEffects::begin()
{
    // Load LED configuration from runtime config
    const RuntimeConfig& config = getRuntimeConfig();
    ledPin = config.ledPin;
    ledCount = config.ledCount;
    ledBrightness = config.ledBrightness;
    ledRefreshRate = config.ledRefreshRate;
    ledUpdateInterval = 1000 / ledRefreshRate;
    ledEffectFadeSpeed = config.ledEffectFadeSpeed;
    ledTwinkleDensity = config.ledTwinkleDensity;
    ledChaseSpeed = config.ledChaseSpeed;
    ledMatrixDrops = config.ledMatrixDrops;
    
    // Allocate arrays based on configuration
    allocateArrays();
    
    LOG_VERBOSE("LEDS", "Initializing LED strip - Pin: %d, Count: %d, Type: WS2812B", ledPin, ledCount);
    
    // Initialize FastLED with WS2812B and GRB color order
    switch(ledPin) {
        case 0:  FastLED.addLeds<WS2812B, 0, GRB>(leds, ledCount); break;
        case 1:  FastLED.addLeds<WS2812B, 1, GRB>(leds, ledCount); break;
        case 2:  FastLED.addLeds<WS2812B, 2, GRB>(leds, ledCount); break;
        case 3:  FastLED.addLeds<WS2812B, 3, GRB>(leds, ledCount); break;
        case 4:  FastLED.addLeds<WS2812B, 4, GRB>(leds, ledCount); break;
        case 5:  FastLED.addLeds<WS2812B, 5, GRB>(leds, ledCount); break;
        case 6:  FastLED.addLeds<WS2812B, 6, GRB>(leds, ledCount); break;
        case 7:  FastLED.addLeds<WS2812B, 7, GRB>(leds, ledCount); break;
        case 8:  FastLED.addLeds<WS2812B, 8, GRB>(leds, ledCount); break;
        case 9:  FastLED.addLeds<WS2812B, 9, GRB>(leds, ledCount); break;
        case 10: FastLED.addLeds<WS2812B, 10, GRB>(leds, ledCount); break;
        default:
            LOG_ERROR("LEDS", "Unsupported LED pin: %d. Supported pins: 0-10", ledPin);
            return false;
    }
    FastLED.setBrightness(ledBrightness);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000); // Limit to 1A at 5V for safety
    
    // Clear all LEDs
    clearAllLEDs();
    FastLED.show();
    
    LOG_NOTICE("LEDS", "LED strip initialized successfully");
    return true;
}

bool LedEffects::reinitialize(int pin, int count, int brightness, int refreshRate,
                             int fadeSpeed, int twinkleDensity, int chaseSpeed, int matrixDrops)
{
    // Stop current effect before reinitializing
    stopEffect();
    
    // Update configuration
    ledPin = pin;
    ledCount = count;
    ledBrightness = brightness;
    ledRefreshRate = refreshRate;
    ledUpdateInterval = 1000 / refreshRate;
    ledEffectFadeSpeed = fadeSpeed;
    ledTwinkleDensity = twinkleDensity;
    ledChaseSpeed = chaseSpeed;
    ledMatrixDrops = matrixDrops;
    
    // Reallocate arrays with new sizes
    allocateArrays();
    
    LOG_VERBOSE("LEDS", "Reinitializing LED strip - Pin: %d, Count: %d", ledPin, ledCount);
    
    // FastLED needs to be cleared and reinitialized
    FastLED.clear();
    
    // Initialize FastLED with WS2812B and GRB color order
    switch(ledPin) {
        case 0:  FastLED.addLeds<WS2812B, 0, GRB>(leds, ledCount); break;
        case 1:  FastLED.addLeds<WS2812B, 1, GRB>(leds, ledCount); break;
        case 2:  FastLED.addLeds<WS2812B, 2, GRB>(leds, ledCount); break;
        case 3:  FastLED.addLeds<WS2812B, 3, GRB>(leds, ledCount); break;
        case 4:  FastLED.addLeds<WS2812B, 4, GRB>(leds, ledCount); break;
        case 5:  FastLED.addLeds<WS2812B, 5, GRB>(leds, ledCount); break;
        case 6:  FastLED.addLeds<WS2812B, 6, GRB>(leds, ledCount); break;
        case 7:  FastLED.addLeds<WS2812B, 7, GRB>(leds, ledCount); break;
        case 8:  FastLED.addLeds<WS2812B, 8, GRB>(leds, ledCount); break;
        case 9:  FastLED.addLeds<WS2812B, 9, GRB>(leds, ledCount); break;
        case 10: FastLED.addLeds<WS2812B, 10, GRB>(leds, ledCount); break;
        default:
            LOG_ERROR("LEDS", "Unsupported LED pin: %d. Supported pins: 0-10", ledPin);
            return false;
    }
    FastLED.setBrightness(ledBrightness);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
    
    // Clear all LEDs
    clearAllLEDs();
    FastLED.show();
    
    LOG_NOTICE("LEDS", "LED strip reinitialized successfully");
    return true;
}

void LedEffects::update()
{
    if (!effectActive) {
        return;
    }
    
    unsigned long now = millis();
    
    // Check if effect has expired
    if (effectDuration > 0 && (now - effectStartTime) >= effectDuration) {
        stopEffect();
        return;
    }
    
    // Rate limiting - only update at specified refresh rate
    if (now - lastUpdate < ledUpdateInterval) {
        return;
    }
    
    lastUpdate = now;
    
    // Update the current effect
    if (currentEffectName.equalsIgnoreCase("simple_chase")) {
        updateSimpleChase();
    } else if (currentEffectName.equalsIgnoreCase("rainbow")) {
        updateRainbowWave();
    } else if (currentEffectName.equalsIgnoreCase("twinkle")) {
        updateTwinkleStars();
    } else if (currentEffectName.equalsIgnoreCase("chase")) {
        updateColorChase();
    } else if (currentEffectName.equalsIgnoreCase("pulse")) {
        updatePulseWave();
    } else if (currentEffectName.equalsIgnoreCase("matrix")) {
        updateMatrixMovie();
    }
    
    FastLED.show();
}

bool LedEffects::startEffect(const String& effectName, unsigned long durationSeconds, 
                            CRGB color1, CRGB color2, CRGB color3)
{
    // Stop current effect
    stopEffect();
    
    // Validate effect name
    String lowerName = effectName;
    lowerName.toLowerCase();
    
    if (!(lowerName.equals("simple_chase") || lowerName.equals("rainbow") || 
          lowerName.equals("twinkle") || lowerName.equals("chase") ||
          lowerName.equals("pulse") || lowerName.equals("matrix"))) {
        LOG_WARNING("LEDS", "Unknown effect name: %s", effectName.c_str());
        return false;
    }
    
    // Set effect parameters
    currentEffectName = lowerName;
    effectColor1 = color1;
    effectColor2 = color2;
    effectColor3 = color3;
    effectStartTime = millis();
    effectDuration = durationSeconds * 1000; // Convert to milliseconds
    effectActive = true;
    
    // Reset effect state
    effectStep = 0;
    effectDirection = 1;
    effectPhase = 0.0f;
    
    // Initialize effect-specific state
    if (lowerName.equals("twinkle")) {
        for (int i = 0; i < ledTwinkleDensity; i++) {
            twinkleStars[i].active = false;
        }
    } else if (lowerName.equals("matrix")) {
        for (int i = 0; i < ledMatrixDrops; i++) {
            matrixDrops[i].active = false;
        }
    }
    
    LOG_VERBOSE("LEDS", "Started effect: %s, duration: %lus", 
                effectName.c_str(), durationSeconds);
    
    return true;
}

void LedEffects::stopEffect()
{
    if (effectActive) {
        LOG_VERBOSE("LEDS", "Stopping effect: %s", currentEffectName.c_str());
    }
    
    effectActive = false;
    currentEffectName = "";
    clearAllLEDs();
    FastLED.show();
}

bool LedEffects::isEffectRunning() const
{
    return effectActive;
}

String LedEffects::getCurrentEffectName() const
{
    return currentEffectName;
}

unsigned long LedEffects::getRemainingTime() const
{
    if (!effectActive || effectDuration == 0) {
        return 0;
    }
    
    unsigned long elapsed = millis() - effectStartTime;
    if (elapsed >= effectDuration) {
        return 0;
    }
    
    return effectDuration - elapsed;
}

void LedEffects::clearAllLEDs()
{
    fill_solid(leds, ledCount, CRGB::Black);
}

void LedEffects::updateSimpleChase()
{
    clearAllLEDs();
    
    // Calculate position based on step
    int position = effectStep % (ledCount * 2); // *2 for off phase
    
    if (position < ledCount) {
        leds[position] = effectColor1;
    }
    // else: off phase (LEDs remain black)
    
    effectStep += ledChaseSpeed;
}

void LedEffects::updateRainbowWave()
{
    for (int i = 0; i < ledCount; i++) {
        // Create rainbow wave with moving phase
        int hue = (i * 255 / ledCount + (int)effectPhase) % 256;
        leds[i] = wheel(hue);
    }
    
    effectPhase += 2.0f; // Speed of wave movement
    if (effectPhase >= 256.0f) {
        effectPhase = 0.0f;
    }
}

void LedEffects::updateTwinkleStars()
{
    // Fade all LEDs slightly
    for (int i = 0; i < ledCount; i++) {
        fadeToBlackBy(i, ledEffectFadeSpeed);
    }
    
    // Update existing twinkle stars
    for (int i = 0; i < ledTwinkleDensity; i++) {
        if (twinkleStars[i].active) {
            // Update brightness
            twinkleStars[i].brightness += twinkleStars[i].fadeDirection * 8;
            
            if (twinkleStars[i].brightness >= 255) {
                twinkleStars[i].brightness = 255;
                twinkleStars[i].fadeDirection = -1;
            } else if (twinkleStars[i].brightness <= 0) {
                twinkleStars[i].brightness = 0;
                twinkleStars[i].active = false;
            }
            
            // Set LED color
            if (twinkleStars[i].active) {
                CRGB color = effectColor1;
                color.fadeToBlackBy(255 - twinkleStars[i].brightness);
                leds[twinkleStars[i].position] = color;
                
                // Light up neighbors with reduced brightness
                if (twinkleStars[i].position > 0) {
                    CRGB neighborColor = effectColor1;
                    neighborColor.fadeToBlackBy(255 - (twinkleStars[i].brightness / 3));
                    leds[twinkleStars[i].position - 1] += neighborColor;
                }
                if (twinkleStars[i].position < ledCount - 1) {
                    CRGB neighborColor = effectColor1;
                    neighborColor.fadeToBlackBy(255 - (twinkleStars[i].brightness / 3));
                    leds[twinkleStars[i].position + 1] += neighborColor;
                }
            }
        }
    }
    
    // Randomly start new twinkle stars
    if (random16(100) < 3) { // 3% chance per update
        for (int i = 0; i < ledTwinkleDensity; i++) {
            if (!twinkleStars[i].active) {
                twinkleStars[i].active = true;
                twinkleStars[i].position = random16(ledCount);
                twinkleStars[i].brightness = 0;
                twinkleStars[i].fadeDirection = 1;
                break;
            }
        }
    }
}

void LedEffects::updateColorChase()
{
    // Continuous chase effect without off phase
    clearAllLEDs();
    
    int position = effectStep % ledCount;
    leds[position] = effectColor1;
    
    // Add trailing dots with fading
    for (int i = 1; i <= 3; i++) {
        int trailPos = (position - i + ledCount) % ledCount;
        CRGB trailColor = effectColor1;
        trailColor.fadeToBlackBy(i * 64); // Fade each trailing dot
        leds[trailPos] = trailColor;
    }
    
    effectStep += ledChaseSpeed;
}

void LedEffects::updatePulseWave()
{
    // Create a sine wave pulse across the strip
    for (int i = 0; i < ledCount; i++) {
        float brightness = sin((effectPhase + i * 0.3f) * 3.14159f / 180.0f);
        brightness = (brightness + 1.0f) / 2.0f; // Normalize to 0-1
        
        CRGB color = effectColor1;
        color.fadeToBlackBy(255 - (int)(brightness * 255));
        leds[i] = color;
    }
    
    effectPhase += 8.0f; // Speed of pulse wave
    if (effectPhase >= 360.0f) {
        effectPhase = 0.0f;
    }
}

void LedEffects::updateMatrixMovie()
{
    // Fade all LEDs
    for (int i = 0; i < ledCount; i++) {
        fadeToBlackBy(i, 64);
    }
    
    // Update existing matrix drops
    for (int i = 0; i < ledMatrixDrops; i++) {
        if (matrixDrops[i].active) {
            // Clear previous position
            for (int j = 0; j < matrixDrops[i].length; j++) {
                int pos = matrixDrops[i].position - j;
                if (pos >= 0 && pos < ledCount) {
                    fadeToBlackBy(pos, 32);
                }
            }
            
            // Move drop down
            matrixDrops[i].position += matrixDrops[i].speed;
            
            // Draw new position
            for (int j = 0; j < matrixDrops[i].length; j++) {
                int pos = matrixDrops[i].position - j;
                if (pos >= 0 && pos < ledCount) {
                    int brightness = 255 - (j * 40); // Fade tail
                    if (brightness > 0) {
                        CRGB color = effectColor1;
                        color.fadeToBlackBy(255 - brightness);
                        leds[pos] = color;
                    }
                }
            }
            
            // Deactivate if off the strip
            if (matrixDrops[i].position >= ledCount + matrixDrops[i].length) {
                matrixDrops[i].active = false;
            }
        }
    }
    
    // Randomly start new drops
    if (random16(100) < 5) { // 5% chance per update
        for (int i = 0; i < ledMatrixDrops; i++) {
            if (!matrixDrops[i].active) {
                matrixDrops[i].active = true;
                matrixDrops[i].position = 0;
                matrixDrops[i].length = random16(8) + 3; // 3-10 LEDs long
                matrixDrops[i].speed = random16(3) + 1;  // 1-3 pixels per update
                break;
            }
        }
    }
}

CRGB LedEffects::wheel(byte wheelPos)
{
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return CRGB(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    if (wheelPos < 170) {
        wheelPos -= 85;
        return CRGB(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    wheelPos -= 170;
    return CRGB(wheelPos * 3, 255 - wheelPos * 3, 0);
}

void LedEffects::fadeToBlackBy(int ledIndex, int fadeValue)
{
    if (ledIndex >= 0 && ledIndex < ledCount) {
        leds[ledIndex].fadeToBlackBy(fadeValue);
    }
}

int LedEffects::random16(int max)
{
    return random(max);
}

#endif // ENABLE_LEDS