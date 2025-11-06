/**
 * @file openai_provider.h
 * @brief OpenAI ChatGPT provider implementation
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

#ifndef OPENAI_PROVIDER_H
#define OPENAI_PROVIDER_H

#include "ai_provider.h"

/**
 * @brief OpenAI ChatGPT provider implementation
 *
 * Supports GPT-4o, GPT-4o-mini, GPT-3.5-turbo models via OpenAI API.
 * Uses thread-safe APIClient singleton for HTTP operations.
 */
class OpenAIProvider : public AIProvider
{
public:
    OpenAIProvider() = default;
    virtual ~OpenAIProvider() = default;

    String generateContent(const String &prompt, const AIProviderConfig &config) override;
    bool testConnection(const String &apiKey, const String &endpoint = "") override;
    const char *getName() const override { return "OpenAI"; }
    AIProviderType getType() const override { return AIProviderType::OPENAI; }
    const char **getSupportedModels(int &count) const override;
    const char *getDefaultModel() const override;

private:
    /**
     * @brief Parse OpenAI API response JSON
     * @param response Raw JSON response from API
     * @return Extracted content string, or empty on error
     */
    String parseResponse(const String &response);
};

#endif // OPENAI_PROVIDER_H
