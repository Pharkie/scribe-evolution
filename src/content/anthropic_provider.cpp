/**
 * @file anthropic_provider.cpp
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

#include "anthropic_provider.h"
#include <utils/api_client.h>
#include <core/logging.h>
#include <ArduinoJson.h>

// Supported Anthropic models
static const char *ANTHROPIC_MODELS[] = {
    "claude-sonnet-4-5-20250929",
    "claude-sonnet-4-20250514",
    "claude-3-5-sonnet-20241022",
    "claude-3-5-haiku-20241022",
    "claude-3-opus-20240229"
};
static const int ANTHROPIC_MODEL_COUNT = sizeof(ANTHROPIC_MODELS) / sizeof(ANTHROPIC_MODELS[0]);

// Default API endpoints
static const char *ANTHROPIC_API_ENDPOINT = "https://api.anthropic.com/v1/messages";
static const char *ANTHROPIC_API_VERSION = "2023-06-01";

String AnthropicProvider::generateContent(const String &prompt, const AIProviderConfig &config)
{
    // Validate configuration
    if (config.apiKey.length() == 0)
    {
        LOG_ERROR("ANTHROPIC", "API key not configured");
        return "";
    }

    // Use provided endpoint or default
    String endpoint = config.endpoint.length() > 0 ? config.endpoint : ANTHROPIC_API_ENDPOINT;

    // Use provided model or default
    String model = config.model.length() > 0 ? config.model : getDefaultModel();

    LOG_VERBOSE("ANTHROPIC", "Calling Anthropic API: %s", endpoint.c_str());
    LOG_VERBOSE("ANTHROPIC", "Using model: %s", model.c_str());
    LOG_VERBOSE("ANTHROPIC", "Using prompt: %s", prompt.c_str());

    // Build JSON payload for Anthropic API
    JsonDocument payloadDoc;
    payloadDoc["model"] = model;
    payloadDoc["max_tokens"] = config.maxTokens;

    // Anthropic doesn't use temperature parameter exactly like OpenAI
    // But we can still include it if needed (optional)

    JsonArray messages = payloadDoc["messages"].to<JsonArray>();
    JsonObject userMessage = messages.add<JsonObject>();
    userMessage["role"] = "user";
    userMessage["content"] = prompt;

    String jsonPayload;
    serializeJson(payloadDoc, jsonPayload);

    // Prepare custom headers for Anthropic API
    const char *headers[][2] = {
        {"x-api-key", config.apiKey.c_str()},
        {"anthropic-version", ANTHROPIC_API_VERSION}
    };
    int headerCount = 2;

    // POST to Anthropic API with custom headers
    String response = APIClient::instance().postToAPIWithCustomHeaders(
        endpoint,
        jsonPayload,
        "ScribeEvolution/1.0",
        headers,
        headerCount,
        config.timeoutMs
    );

    if (response.length() == 0)
    {
        LOG_ERROR("ANTHROPIC", "No response from Anthropic API");
        return "";
    }

    LOG_VERBOSE("ANTHROPIC", "API response received: %s", response.c_str());

    // Parse response and return content
    return parseResponse(response);
}

bool AnthropicProvider::testConnection(const String &apiKey, const String &endpoint)
{
    if (apiKey.length() == 0)
    {
        LOG_ERROR("ANTHROPIC", "Cannot test connection: API key not provided");
        return false;
    }

    // For Anthropic, we can test by making a simple request with minimal tokens
    String testEndpoint = endpoint.length() > 0 ? endpoint : ANTHROPIC_API_ENDPOINT;

    LOG_VERBOSE("ANTHROPIC", "Testing connection to: %s", testEndpoint.c_str());

    // Build minimal test payload
    JsonDocument payloadDoc;
    payloadDoc["model"] = getDefaultModel();
    payloadDoc["max_tokens"] = 10; // Minimal tokens for test

    JsonArray messages = payloadDoc["messages"].to<JsonArray>();
    JsonObject userMessage = messages.add<JsonObject>();
    userMessage["role"] = "user";
    userMessage["content"] = "test";

    String jsonPayload;
    serializeJson(payloadDoc, jsonPayload);

    // Prepare custom headers
    const char *headers[][2] = {
        {"x-api-key", apiKey.c_str()},
        {"anthropic-version", ANTHROPIC_API_VERSION}
    };
    int headerCount = 2;

    String response = APIClient::instance().postToAPIWithCustomHeaders(
        testEndpoint,
        jsonPayload,
        "ScribeEvolution/1.0",
        headers,
        headerCount,
        5000 // 5 second timeout
    );

    if (response.length() == 0)
    {
        LOG_ERROR("ANTHROPIC", "Connection test failed: no response");
        return false;
    }

    // Parse response to verify it's valid JSON with expected structure
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        LOG_ERROR("ANTHROPIC", "Connection test failed: invalid JSON response");
        return false;
    }

    // Check if response contains "content" array (valid message response)
    if (!doc["content"].is<JsonArray>())
    {
        LOG_ERROR("ANTHROPIC", "Connection test failed: unexpected response format");
        return false;
    }

    LOG_VERBOSE("ANTHROPIC", "Connection test successful");
    return true;
}

const char **AnthropicProvider::getSupportedModels(int &count) const
{
    count = ANTHROPIC_MODEL_COUNT;
    return ANTHROPIC_MODELS;
}

const char *AnthropicProvider::getDefaultModel() const
{
    return "claude-sonnet-4-5-20250929";
}

String AnthropicProvider::parseResponse(const String &response)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        LOG_ERROR("ANTHROPIC", "Response parsing failed: %s", error.c_str());
        return "";
    }

    // Validate response structure
    if (!doc["content"].is<JsonArray>() || doc["content"].size() == 0)
    {
        LOG_ERROR("ANTHROPIC", "Response missing content array");
        LOG_ERROR("ANTHROPIC", "Response was: %s", response.c_str());
        return "";
    }

    JsonArray contentArray = doc["content"];

    // Find the first text content block
    for (JsonVariant item : contentArray)
    {
        if (item["type"].as<String>() == "text" && item["text"].is<JsonVariant>())
        {
            String content = item["text"].as<String>();
            content.trim();

            if (content.length() == 0)
            {
                LOG_ERROR("ANTHROPIC", "Anthropic returned empty content");
                return "";
            }

            LOG_VERBOSE("ANTHROPIC", "Extracted content: %s", content.c_str());
            return content;
        }
    }

    LOG_ERROR("ANTHROPIC", "No text content found in response");
    LOG_ERROR("ANTHROPIC", "Response was: %s", response.c_str());
    return "";
}
