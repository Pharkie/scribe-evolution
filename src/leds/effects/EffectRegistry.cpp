/**
 * @file EffectRegistry.cpp
 * @brief Implementation of LED effects registry
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "EffectRegistry.h"

#ifdef ENABLE_LEDS

#include "../../core/logging.h"

// Static effect names array
const String EffectRegistry::effectNames[] = {
    "chase_single",
    "rainbow",
    "twinkle",
    "chase_multi",
    "pulse",
    "matrix"};

const int EffectRegistry::numEffects = 6;

EffectRegistry::EffectRegistry(int chaseSpeed, int twinkleDensity, int fadeSpeed, int matrixDrops)
    : chaseSpeed(chaseSpeed), twinkleDensity(twinkleDensity), fadeSpeed(fadeSpeed), matrixDrops(matrixDrops)
{
}

EffectRegistry::~EffectRegistry()
{
    // Effects are managed by the caller
}

EffectBase *EffectRegistry::createEffect(const String &effectName)
{
    String lowerName = toLowerCase(effectName);

    if (lowerName.equals("chase_single"))
    {
        return new ChaseSingle(chaseSpeed);
    }
    else if (lowerName.equals("rainbow"))
    {
        return new RainbowWave();
    }
    else if (lowerName.equals("twinkle"))
    {
        return new TwinkleStars(twinkleDensity, fadeSpeed);
    }
    else if (lowerName.equals("chase_multi"))
    {
        return new ChaseMulti(chaseSpeed);
    }
    else if (lowerName.equals("pulse"))
    {
        return new PulseWave();
    }
    else if (lowerName.equals("matrix"))
    {
        return new Matrix(matrixDrops);
    }

    LOG_WARNING("LEDS", "Unknown effect name: %s", effectName.c_str());
    return nullptr;
}

bool EffectRegistry::isValidEffect(const String &effectName) const
{
    String lowerName = toLowerCase(effectName);

    for (int i = 0; i < numEffects; i++)
    {
        if (lowerName.equals(effectNames[i]))
        {
            return true;
        }
    }
    return false;
}

String EffectRegistry::getAvailableEffects() const
{
    String result = "";
    for (int i = 0; i < numEffects; i++)
    {
        if (i > 0)
            result += ", ";
        result += effectNames[i];
    }
    return result;
}

void EffectRegistry::updateConfig(int newChaseSpeed, int newTwinkleDensity, int newFadeSpeed, int newMatrixDrops)
{
    chaseSpeed = newChaseSpeed;
    twinkleDensity = newTwinkleDensity;
    fadeSpeed = newFadeSpeed;
    matrixDrops = newMatrixDrops;
}

String EffectRegistry::toLowerCase(const String &str) const
{
    String result = str;
    result.toLowerCase();
    return result;
}

#endif // ENABLE_LEDS
