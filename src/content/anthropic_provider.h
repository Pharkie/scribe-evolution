/**
 * @file anthropic_provider.h
 * @brief Anthropic Claude provider implementation
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

#ifndef ANTHROPIC_PROVIDER_H
#define ANTHROPIC_PROVIDER_H

#include "ai_provider.h"

/**
 * @brief Anthropic Claude provider implementation
 *
 * Supports Claude 3.5 Sonnet, Claude 3 Haiku, Claude 3 Opus models via Anthropic API.
 * Uses thread-safe APIClient singleton for HTTP operations.
 */
class AnthropicProvider : public AIProvider
{
public:
    AnthropicProvider() = default;
    virtual ~AnthropicProvider() = default;

    String generateContent(const String &prompt, const AIProviderConfig &config) override;
    bool testConnection(const String &apiKey, const String &endpoint = "") override;
    const char *getName() const override { return "Anthropic"; }
    AIProviderType getType() const override { return AIProviderType::ANTHROPIC; }
    const char **getSupportedModels(int &count) const override;
    const char *getDefaultModel() const override;

private:
    /**
     * @brief Parse Anthropic API response JSON
     * @param response Raw JSON response from API
     * @return Extracted content string, or empty on error
     */
    String parseResponse(const String &response);
};

#endif // ANTHROPIC_PROVIDER_H
