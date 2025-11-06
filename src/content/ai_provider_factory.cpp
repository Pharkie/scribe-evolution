/**
 * @file ai_provider_factory.cpp
 * @brief Factory for creating AI provider instances
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 */

#include "ai_provider_factory.h"
#include "openai_provider.h"
#include "anthropic_provider.h"
#include "google_provider.h"
#include <core/logging.h>

AIProvider *AIProviderFactory::createProvider(AIProviderType type)
{
    switch (type)
    {
    case AIProviderType::OPENAI:
        LOG_VERBOSE("AIFACTORY", "Creating OpenAI provider");
        return new OpenAIProvider();

    case AIProviderType::ANTHROPIC:
        LOG_VERBOSE("AIFACTORY", "Creating Anthropic provider");
        return new AnthropicProvider();

    case AIProviderType::GOOGLE:
        LOG_VERBOSE("AIFACTORY", "Creating Google provider");
        return new GoogleProvider();

    case AIProviderType::LOCAL:
        LOG_ERROR("AIFACTORY", "LOCAL provider type not yet implemented");
        return nullptr;

    default:
        LOG_ERROR("AIFACTORY", "Unknown provider type: %d", static_cast<int>(type));
        return nullptr;
    }
}

const char *AIProviderFactory::getProviderName(AIProviderType type)
{
    switch (type)
    {
    case AIProviderType::OPENAI:
        return "OpenAI";
    case AIProviderType::ANTHROPIC:
        return "Anthropic";
    case AIProviderType::GOOGLE:
        return "Google";
    case AIProviderType::LOCAL:
        return "Local";
    default:
        return "Unknown";
    }
}

AIProviderType AIProviderFactory::parseProviderType(const String &name)
{
    String lowerName = name;
    lowerName.toLowerCase();

    if (lowerName == "openai")
    {
        return AIProviderType::OPENAI;
    }
    else if (lowerName == "anthropic")
    {
        return AIProviderType::ANTHROPIC;
    }
    else if (lowerName == "google")
    {
        return AIProviderType::GOOGLE;
    }
    else if (lowerName == "local")
    {
        return AIProviderType::LOCAL;
    }
    else
    {
        LOG_WARNING("AIFACTORY", "Unknown provider name '%s', defaulting to OpenAI", name.c_str());
        return AIProviderType::OPENAI;
    }
}
