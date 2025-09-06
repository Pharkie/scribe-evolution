/**
 * @file Matrix.cpp
 * @brief Implementation of Matrix-style falling code effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "Matrix.h"

#if ENABLE_LEDS

#include <core/led_config.h>

Matrix::Matrix(const MatrixConfig &config)
    : config(config), matrixDrops(nullptr), initialized(false), frameCounter(0), hadActiveSinceCycleStart(false), spawnedThisCycle(0), allowSpawning(true)
{
}

Matrix::~Matrix()
{
    deallocateDrops();
}

void Matrix::initialize(int ledCount)
{
    deallocateDrops(); // Clean up any existing allocation

    if (config.drops > 0)
    {
        matrixDrops = new MatrixDrop[config.drops];
        for (int i = 0; i < config.drops; i++)
        {
            matrixDrops[i].active = false;
            matrixDrops[i].position = 0;
            matrixDrops[i].length = 0;
            matrixDrops[i].speed = 1;
            matrixDrops[i].completedCycle = false;
        }
        initialized = true;
    }
    // Reset cycle tracking
    spawnedThisCycle = 0;
    allowSpawning = true;
    hadActiveSinceCycleStart = false;
}

bool Matrix::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                    float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                    int &completedCycles)
{
    if (!initialized || !matrixDrops)
    {
        return true; // Continue running but do nothing
    }

    // Fade all LEDs
    for (int i = 0; i < ledCount; i++)
    {
        fadeToBlackBy(leds, i, config.backgroundFade);
    }

    // Use frame counter for speed control.
    // config.speed represents a frame delay (smaller = faster)
    frameCounter++;
    if (frameCounter >= config.speed)
    {
        frameCounter = 0;

        // Update existing matrix drops
        for (int i = 0; i < config.drops; i++)
        {
            if (matrixDrops[i].active)
            {
                hadActiveSinceCycleStart = true; // Mark that we had an active drop in this cycle window
                // Clear previous position
                for (int j = 0; j < matrixDrops[i].length; j++)
                {
                    int pos = matrixDrops[i].position - j;
                    if (pos >= 0 && pos < ledCount)
                    {
                        fadeToBlackBy(leds, pos, config.trailFade);
                    }
                }

                // Move drop down
                matrixDrops[i].position += matrixDrops[i].speed;

                // Deactivate if off the strip
                if (matrixDrops[i].position >= ledCount + matrixDrops[i].length)
                {
                    matrixDrops[i].active = false;
                    matrixDrops[i].completedCycle = true;
                }
            }
        }

        // Randomly start new drops (only if we still have quota in this cycle)
        if (allowSpawning && random16(100) < 8)
        { // 8% chance per update (increased from 5% for more activity)
            for (int i = 0; i < config.drops; i++)
            {
                if (!matrixDrops[i].active)
                {
                    matrixDrops[i].active = true;
                    matrixDrops[i].position = 0;
                    matrixDrops[i].length = random16(8) + 3; // 3-10 LEDs long
                    matrixDrops[i].speed = random16(3) + 1;  // 1-3 pixels per update
                    matrixDrops[i].completedCycle = false;   // Reset cycle flag
                    spawnedThisCycle++;
                    if (spawnedThisCycle >= config.drops)
                    {
                        allowSpawning = false; // Stop spawning; wait for all to finish
                    }
                    break;
                }
            }
        }
    }

    // Draw current positions (always draw, even if not updating movement)
    for (int i = 0; i < config.drops; i++)
    {
        if (matrixDrops[i].active)
        {
            // Draw new position with green color (Matrix-style)
            for (int j = 0; j < matrixDrops[i].length; j++)
            {
                int pos = matrixDrops[i].position - j;
                if (pos >= 0 && pos < ledCount)
                {
                    int brightness = 255 - (j * config.brightnessFade); // Fade tail
                    if (brightness > 0)
                    {
                        // Use color1 parameter (should be green from web interface)
                        CRGB color = color1;
                        color.fadeToBlackBy(255 - brightness);
                        leds[pos] = color;
                    }
                }
            }
        }
    }

    // At natural boundary: if no drops are active and we spawned our quota,
    // consider one full cycle complete (all drops have exited the strip)
    bool anyActive = false;
    for (int i = 0; i < config.drops; i++)
    {
        if (matrixDrops[i].active)
        {
            anyActive = true;
            break;
        }
    }
    if (!anyActive && !allowSpawning && spawnedThisCycle >= config.drops)
    {
        completedCycles++;
        // Reset for next cycle
        hadActiveSinceCycleStart = false;
        spawnedThisCycle = 0;
        allowSpawning = true;
    }

    return true; // Continue running (cycle boundary handled above)
}

void Matrix::reset()
{
    frameCounter = 0;
    if (matrixDrops)
    {
        for (int i = 0; i < config.drops; i++)
        {
            matrixDrops[i].active = false;
            matrixDrops[i].position = 0;
            matrixDrops[i].length = 0;
            matrixDrops[i].speed = 1;
            matrixDrops[i].completedCycle = false;
        }
    }
}

void Matrix::setDrops(int newDrops)
{
    if (newDrops != config.drops)
    {
        config.drops = newDrops;
        initialized = false; // Force reinitialization
    }
}

void Matrix::deallocateDrops()
{
    if (matrixDrops)
    {
        delete[] matrixDrops;
        matrixDrops = nullptr;
        initialized = false;
    }
}

void Matrix::fadeToBlackBy(CRGB *leds, int ledIndex, int fadeValue)
{
    leds[ledIndex].fadeToBlackBy(fadeValue);
}

int Matrix::random16(int max)
{
    return random(max);
}

#endif // ENABLE_LEDS
