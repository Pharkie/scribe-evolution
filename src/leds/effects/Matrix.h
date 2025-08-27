/**
 * @file Matrix.h
 * @brief Matrix-style falling code effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef MATRIX_H
#define MATRIX_H

#include "EffectBase.h"

#if ENABLE_LEDS

/**
 * @brief Matrix-style falling code effect
 * Creates falling drops of light with trailing fades like the Matrix movie
 */
class Matrix : public EffectBase
{
public:
    /**
     * @brief Constructor
     * @param config Configuration for the matrix effect
     */
    Matrix(const MatrixConfig &config);

    /**
     * @brief Destructor - cleanup allocated memory
     */
    ~Matrix();

    /**
     * @brief Initialize effect with LED count
     */
    void initialize(int ledCount) override;

    /**
     * @brief Update the matrix movie effect
     */
    bool update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                int &completedCycles) override;

    /**
     * @brief Reset effect state
     */
    void reset() override;

    /**
     * @brief Get effect name
     */
    String getName() const override { return "matrix"; }

    /**
     * @brief Get effect name
     */

    /**
     * @brief Set number of drops
     * @param drops Number of simultaneous matrix drops
     */
    void setDrops(int drops);

private:
    MatrixConfig config; // Store the autonomous configuration
    struct MatrixDrop
    {
        int position;
        int length;
        int speed;
        bool active;
        bool completedCycle; // Track if this drop has completed a full cycle
    };

    MatrixDrop *matrixDrops;
    bool initialized;
    int frameCounter; // Frame counter for speed control

    void deallocateDrops();
    void fadeToBlackBy(CRGB *leds, int ledIndex, int fadeValue);
    int random16(int max);
};

#endif // ENABLE_LEDS
#endif // MATRIX_H
