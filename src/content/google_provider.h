/**
 * @file google_provider.h
 * @brief Google Gemini provider implementation
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

#ifndef GOOGLE_PROVIDER_H
#define GOOGLE_PROVIDER_H

#include "ai_provider.h"

/**
 * @brief Google Gemini provider implementation
 *
 * Supports Gemini 1.5 Flash, Gemini 1.5 Pro models via Google AI API.
 * Uses thread-safe APIClient singleton for HTTP operations.
 */
class GoogleProvider : public AIProvider
{
public:
    GoogleProvider() = default;
    virtual ~GoogleProvider() = default;

    String generateContent(const String &prompt, const AIProviderConfig &config) override;
    bool testConnection(const String &apiKey, const String &endpoint = "") override;
    const char *getName() const override { return "Google"; }
    AIProviderType getType() const override { return AIProviderType::GOOGLE; }
    const char **getSupportedModels(int &count) const override;
    const char *getDefaultModel() const override;

private:
    /**
     * @brief Parse Google Gemini API response JSON
     * @param response Raw JSON response from API
     * @return Extracted content string, or empty on error
     */
    String parseResponse(const String &response);

    /**
     * @brief Build Gemini API endpoint URL with model and API key
     * @param model Model name
     * @param apiKey API key
     * @param baseEndpoint Base endpoint URL (optional)
     * @return Complete endpoint URL with API key
     */
    String buildEndpointURL(const String &model, const String &apiKey, const String &baseEndpoint = "");
};

#endif // GOOGLE_PROVIDER_H
