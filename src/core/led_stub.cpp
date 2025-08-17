/**
 * @file led_stub.cpp
 * @brief Stub implementation for LED functionality when LEDs are disabled
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "../core/config.h"

#if !ENABLE_LEDS

// Simple LED stub class to satisfy external references when LEDs are disabled
class LedEffects
{
public:
    bool begin() { return true; } // Always succeed but do nothing
    bool startEffectCycles(const String &effectName, int cycles = 1,
                           unsigned long color1 = 0, unsigned long color2 = 0, unsigned long color3 = 0)
    {
        return true;
    }
    void update() {} // Do nothing
    void stopEffect() {}
    bool isEffectRunning() const { return false; }
    String getCurrentEffectName() const { return ""; }
};

// Global instance when LEDs are disabled
LedEffects ledEffects;

#endif // !ENABLE_LEDS
