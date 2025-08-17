/**
 * @file Matrix.cpp
 * @brief Implementation of Matrix-style falling code effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "Matrix.h"

#ifdef ENABLE_LEDS

Matrix::Matrix(int drops)
    : matrixDrops(nullptr), drops(drops), initialized(false), frameCounter(0)
{
}

Matrix::~Matrix()
{
    deallocateDrops();
}

void Matrix::initialize(int ledCount)
{
    deallocateDrops(); // Clean up any existing allocation

    if (drops > 0)
    {
        matrixDrops = new MatrixDrop[drops];
        for (int i = 0; i < drops; i++)
        {
            matrixDrops[i].active = false;
            matrixDrops[i].position = 0;
            matrixDrops[i].length = 0;
            matrixDrops[i].speed = 1;
        }
        initialized = true;
    }
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
        fadeToBlackBy(leds, i, MATRIX_BACKGROUND_FADE);
    }

    // Use frame counter for speed control - update only every few frames
    frameCounter++;
    if (frameCounter >= 3) // Update every 3 frames for slower matrix effect
    {
        frameCounter = 0;

        // Update existing matrix drops
        for (int i = 0; i < drops; i++)
        {
            if (matrixDrops[i].active)
            {
                // Clear previous position
                for (int j = 0; j < matrixDrops[i].length; j++)
                {
                    int pos = matrixDrops[i].position - j;
                    if (pos >= 0 && pos < ledCount)
                    {
                        fadeToBlackBy(leds, pos, MATRIX_TRAIL_FADE);
                    }
                }

                // Move drop down
                matrixDrops[i].position += matrixDrops[i].speed;

                // Deactivate if off the strip
                if (matrixDrops[i].position >= ledCount + matrixDrops[i].length)
                {
                    matrixDrops[i].active = false;
                }
            }
        }

        // Randomly start new drops
        if (random16(100) < 8)
        { // 8% chance per update (increased from 5% for more activity)
            for (int i = 0; i < drops; i++)
            {
                if (!matrixDrops[i].active)
                {
                    matrixDrops[i].active = true;
                    matrixDrops[i].position = 0;
                    matrixDrops[i].length = random16(8) + 3; // 3-10 LEDs long
                    matrixDrops[i].speed = random16(3) + 1;  // 1-3 pixels per update
                    break;
                }
            }
        }
    }

    // Draw current positions (always draw, even if not updating movement)
    for (int i = 0; i < drops; i++)
    {
        if (matrixDrops[i].active)
        {
            // Draw new position with green color (Matrix-style)
            for (int j = 0; j < matrixDrops[i].length; j++)
            {
                int pos = matrixDrops[i].position - j;
                if (pos >= 0 && pos < ledCount)
                {
                    int brightness = 255 - (j * MATRIX_BRIGHTNESS_FADE_STEP); // Fade tail
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

    return true; // Continue running (duration-based effect)
}

void Matrix::reset()
{
    frameCounter = 0;
    if (matrixDrops)
    {
        for (int i = 0; i < drops; i++)
        {
            matrixDrops[i].active = false;
            matrixDrops[i].position = 0;
            matrixDrops[i].length = 0;
            matrixDrops[i].speed = 1;
        }
    }
}

void Matrix::setDrops(int newDrops)
{
    if (newDrops != drops)
    {
        drops = newDrops;
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
