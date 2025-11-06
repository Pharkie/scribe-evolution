/**
 * @file ai_provider.h
 * @brief Abstract interface for AI content generation providers
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

#ifndef AI_PROVIDER_H
#define AI_PROVIDER_H

#include <Arduino.h>

/**
 * @brief AI provider types enumeration
 */
enum class AIProviderType
{
    OPENAI,    // OpenAI ChatGPT (GPT-4o, GPT-4o-mini, GPT-3.5-turbo)
    ANTHROPIC, // Anthropic Claude (Claude 3.5 Sonnet, Claude 3 Haiku)
    GOOGLE,    // Google Gemini (Gemini 1.5 Flash, Gemini 1.5 Pro)
    LOCAL      // Local LLM via HTTP endpoint
};

/**
 * @brief Configuration structure for AI provider settings
 */
struct AIProviderConfig
{
    String apiKey;        // API key for authentication
    String model;         // Model name (provider-specific)
    String endpoint;      // API endpoint URL (provider-specific)
    float temperature;    // Sampling temperature (0.0 - 2.0)
    int maxTokens;        // Maximum tokens to generate
    int timeoutMs;        // Request timeout in milliseconds
};

/**
 * @brief Abstract base class for AI content generation providers
 *
 * All AI providers must implement this interface to be used by the
 * content generation system. Providers handle their own API communication,
 * authentication, and response parsing.
 */
class AIProvider
{
public:
    virtual ~AIProvider() = default;

    /**
     * @brief Generate content using the AI provider
     * @param prompt The prompt to send to the AI
     * @param config Provider configuration (API key, model, temperature, etc.)
     * @return String containing generated content, or empty string on failure
     */
    virtual String generateContent(const String &prompt, const AIProviderConfig &config) = 0;

    /**
     * @brief Test connection to the AI provider API
     * @param apiKey API key to test
     * @param endpoint Optional custom endpoint to test
     * @return true if connection successful, false otherwise
     */
    virtual bool testConnection(const String &apiKey, const String &endpoint = "") = 0;

    /**
     * @brief Get the provider name
     * @return String containing provider name (e.g., "OpenAI", "Anthropic")
     */
    virtual const char *getName() const = 0;

    /**
     * @brief Get the provider type
     * @return AIProviderType enum value
     */
    virtual AIProviderType getType() const = 0;

    /**
     * @brief Get supported models for this provider
     * @return Array of model names
     */
    virtual const char **getSupportedModels(int &count) const = 0;

    /**
     * @brief Get default model for this provider
     * @return String containing default model name
     */
    virtual const char *getDefaultModel() const = 0;
};

#endif // AI_PROVIDER_H
