/**
 * @file ai_provider_factory.h
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

#ifndef AI_PROVIDER_FACTORY_H
#define AI_PROVIDER_FACTORY_H

#include "ai_provider.h"

/**
 * @brief Factory class for creating AI provider instances
 *
 * Uses factory pattern to instantiate the correct provider based on
 * AIProviderType. Returns nullptr for unsupported or LOCAL type.
 *
 * Example usage:
 * ```cpp
 * AIProvider* provider = AIProviderFactory::createProvider(AIProviderType::OPENAI);
 * if (provider) {
 *     String content = provider->generateContent(prompt, config);
 *     delete provider;
 * }
 * ```
 */
class AIProviderFactory
{
public:
    /**
     * @brief Create an AI provider instance
     * @param type The provider type to create
     * @return Pointer to AIProvider instance, or nullptr if type is unsupported
     * @note Caller is responsible for deleting the returned pointer
     */
    static AIProvider *createProvider(AIProviderType type);

    /**
     * @brief Get provider name string from type
     * @param type The provider type
     * @return Human-readable provider name
     */
    static const char *getProviderName(AIProviderType type);

    /**
     * @brief Parse provider type from string
     * @param name Provider name string (case-insensitive: "openai", "anthropic", "google", "local")
     * @return AIProviderType enum value, defaults to OPENAI if unknown
     */
    static AIProviderType parseProviderType(const String &name);
};

#endif // AI_PROVIDER_FACTORY_H
