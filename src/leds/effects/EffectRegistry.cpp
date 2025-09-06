/**
 * @file EffectRegistry.cpp
 * @brief Implementation of LED effects registry
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "EffectRegistry.h"

#if ENABLE_LEDS

#include <core/logging.h>
// No color conversion here; expose hex defaults via getDefaultColorsHex

// Static effect names array
const String EffectRegistry::effectNames[] = {
    "chase_single",
    "rainbow",
    "twinkle",
    "chase_multi",
    "pulse",
    "matrix"};

const int EffectRegistry::numEffects = 6;

EffectRegistry::EffectRegistry(const LedEffectsConfig &config)
    : effectsConfig(config)
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
        return new ChaseSingle(effectsConfig.chaseSingle);
    }
    else if (lowerName.equals("rainbow"))
    {
        return new RainbowWave(effectsConfig.rainbow);
    }
    else if (lowerName.equals("twinkle"))
    {
        return new TwinkleStars(effectsConfig.twinkle);
    }
    else if (lowerName.equals("chase_multi"))
    {
        return new ChaseMulti(effectsConfig.chaseMulti);
    }
    else if (lowerName.equals("pulse"))
    {
        return new PulseWave(effectsConfig.pulse);
    }
    else if (lowerName.equals("matrix"))
    {
        return new Matrix(effectsConfig.matrix);
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

void EffectRegistry::updateConfig(const LedEffectsConfig &newEffectsConfig)
{
    effectsConfig = newEffectsConfig;
}

String EffectRegistry::toLowerCase(const String &str) const
{
    String result = str;
    result.toLowerCase();
    return result;
}

void EffectRegistry::getDefaultColorsHex(const String &effectName, String &h1, String &h2, String &h3) const
{
    h1 = "";
    h2 = "";
    h3 = "";

    String lower = toLowerCase(effectName);

    if (lower.equals("chase_single"))
    {
        h1 = effectsConfig.chaseSingle.defaultColor;
    }
    else if (lower.equals("chase_multi"))
    {
        h1 = effectsConfig.chaseMulti.color1;
        h2 = effectsConfig.chaseMulti.color2;
        h3 = effectsConfig.chaseMulti.color3;
    }
    else if (lower.equals("matrix"))
    {
        h1 = effectsConfig.matrix.defaultColor;
    }
    else if (lower.equals("twinkle"))
    {
        h1 = effectsConfig.twinkle.defaultColor;
    }
    else if (lower.equals("pulse"))
    {
        h1 = effectsConfig.pulse.defaultColor;
    }
    else if (lower.equals("rainbow"))
    {
        // Rainbow ignores colors; use white to ensure visible if used
        h1 = "#FFFFFF";
    }
}

#endif // ENABLE_LEDS
