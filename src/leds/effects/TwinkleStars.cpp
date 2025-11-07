/**
 * @file TwinkleStars.cpp
 * @brief Implementation of twinkling stars effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "TwinkleStars.h"

#if ENABLE_LEDS

#include <core/led_config.h>

TwinkleStars::TwinkleStars(const TwinkleConfig &config)
    : config(config), twinkleStars(nullptr), initialized(false), timeAccumulator(0.0f)
{
    initTiming();
}

TwinkleStars::~TwinkleStars()
{
    deallocateStars();
}

void TwinkleStars::initialize(int ledCount)
{
    deallocateStars(); // Clean up any existing allocation

    if (config.density > 0)
    {
        twinkleStars = new TwinkleState[config.density];
        for (int i = 0; i < config.density; i++)
        {
            twinkleStars[i].active = false;
            twinkleStars[i].position = 0;
            twinkleStars[i].brightness = 0;
            twinkleStars[i].fadeDirection = 1;
        }
        initialized = true;
    }
}

bool TwinkleStars::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                          float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                          int &completedCycles)
{
    if (!initialized || !twinkleStars)
    {
        return true; // Continue running but do nothing
    }

    float deltaTime = calculateDeltaTime();

    // Update ~30 times per second (time-based instead of frame-based)
    const float updateInterval = 1.0f / 30.0f; // 33.33ms
    timeAccumulator += deltaTime;

    if (timeAccumulator >= updateInterval)
    {
        timeAccumulator -= updateInterval;

        // Update existing twinkle stars
        for (int i = 0; i < config.density; i++)
        {
            if (twinkleStars[i].active)
            {
                // Update brightness
                twinkleStars[i].brightness += twinkleStars[i].fadeDirection * 8;

                if (twinkleStars[i].brightness >= 255)
                {
                    twinkleStars[i].brightness = 255;
                    twinkleStars[i].fadeDirection = -1;
                }
                else if (twinkleStars[i].brightness <= 0)
                {
                    twinkleStars[i].brightness = 0;
                    twinkleStars[i].active = false;
                    // Count completed twinkle as a cycle
                    completedCycles++;
                }
            }
        }

        // Randomly start new twinkle stars; keep spawn conservative to avoid rapid flicker
        int spawnChance = 1 + (max(1, config.fadeSpeed) * 6 / 64); // ~1%..7%
        if (random16(100) < spawnChance)
        {
            for (int i = 0; i < config.density; i++)
            {
                if (!twinkleStars[i].active)
                {
                    twinkleStars[i].active = true;
                    twinkleStars[i].position = random16(ledCount);
                    twinkleStars[i].brightness = 0;
                    twinkleStars[i].fadeDirection = 1;
                    break;
                }
            }
        }

        // Single-pass rendering: fade background and draw stars
        // First, fade all LEDs for background
        int fadeStep = max(1, config.fadeSpeed);
        for (int i = 0; i < ledCount; i++)
        {
            leds[i].fadeToBlackBy(fadeStep);
        }

        // Then draw all active stars (overwrites background fade for active pixels)
        for (int i = 0; i < config.density; i++)
        {
            if (twinkleStars[i].active)
            {
                // Use color1 parameter (should be white from web interface)
                CRGB color = color1;
                color.fadeToBlackBy(255 - twinkleStars[i].brightness);
                leds[twinkleStars[i].position] = color;

                // Light up neighbors with reduced brightness
                if (twinkleStars[i].position > 0)
                {
                    CRGB neighborColor = color1;
                    neighborColor.fadeToBlackBy(255 - (twinkleStars[i].brightness / 3));
                    leds[twinkleStars[i].position - 1] += neighborColor;
                }
                if (twinkleStars[i].position < ledCount - 1)
                {
                    CRGB neighborColor = color1;
                    neighborColor.fadeToBlackBy(255 - (twinkleStars[i].brightness / 3));
                    leds[twinkleStars[i].position + 1] += neighborColor;
                }
            }
        }
    }

    return true; // Continue running (duration-based effect)
}

void TwinkleStars::reset()
{
    EffectBase::reset();
    timeAccumulator = 0.0f;
    if (twinkleStars)
    {
        for (int i = 0; i < config.density; i++)
        {
            twinkleStars[i].active = false;
            twinkleStars[i].position = 0;
            twinkleStars[i].brightness = 0;
            twinkleStars[i].fadeDirection = 1;
        }
    }
}

void TwinkleStars::setDensity(int newDensity)
{
    if (newDensity != config.density)
    {
        config.density = newDensity;
        initialized = false; // Force reinitialization
    }
}

void TwinkleStars::deallocateStars()
{
    if (twinkleStars)
    {
        delete[] twinkleStars;
        twinkleStars = nullptr;
        initialized = false;
    }
}

int TwinkleStars::random16(int max)
{
    return random(max);
}

#endif // ENABLE_LEDS
