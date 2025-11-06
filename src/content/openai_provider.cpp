/**
 * @file openai_provider.cpp
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

#include "openai_provider.h"
#include <utils/api_client.h>
#include <core/logging.h>
#include <ArduinoJson.h>

// Supported OpenAI models
static const char *OPENAI_MODELS[] = {
    "gpt-4o-mini",
    "gpt-4o",
    "o1",
    "o1-mini",
    "gpt-4-turbo",
    "gpt-3.5-turbo"
};
static const int OPENAI_MODEL_COUNT = sizeof(OPENAI_MODELS) / sizeof(OPENAI_MODELS[0]);

// Default API endpoints
static const char *OPENAI_API_ENDPOINT = "https://api.openai.com/v1/chat/completions";
static const char *OPENAI_TEST_ENDPOINT = "https://api.openai.com/v1/models";

String OpenAIProvider::generateContent(const String &prompt, const AIProviderConfig &config)
{
    // Validate configuration
    if (config.apiKey.length() == 0)
    {
        LOG_ERROR("OPENAI", "API key not configured");
        return "";
    }

    // Use provided endpoint or default
    String endpoint = config.endpoint.length() > 0 ? config.endpoint : OPENAI_API_ENDPOINT;

    // Use provided model or default
    String model = config.model.length() > 0 ? config.model : getDefaultModel();

    // Build Bearer token with automatic prefix
    String bearerToken = config.apiKey;
    if (!bearerToken.startsWith("Bearer "))
    {
        bearerToken = "Bearer " + bearerToken;
    }

    LOG_VERBOSE("OPENAI", "Calling OpenAI API: %s", endpoint.c_str());
    LOG_VERBOSE("OPENAI", "Using model: %s", model.c_str());
    LOG_VERBOSE("OPENAI", "Using prompt: %s", prompt.c_str());

    // Build JSON payload for OpenAI ChatGPT API
    JsonDocument payloadDoc;
    payloadDoc["model"] = model;
    payloadDoc["max_tokens"] = config.maxTokens;
    payloadDoc["temperature"] = config.temperature;

    JsonArray messages = payloadDoc["messages"].to<JsonArray>();
    JsonObject userMessage = messages.add<JsonObject>();
    userMessage["role"] = "user";
    userMessage["content"] = prompt;

    String jsonPayload;
    serializeJson(payloadDoc, jsonPayload);

    // POST to OpenAI API with Bearer token
    String response = postToAPIWithBearer(
        endpoint,
        bearerToken,
        jsonPayload,
        "ScribeEvolution/1.0",
        config.timeoutMs
    );

    if (response.length() == 0)
    {
        LOG_ERROR("OPENAI", "No response from OpenAI API");
        return "";
    }

    LOG_VERBOSE("OPENAI", "API response received: %s", response.c_str());

    // Parse response and return content
    return parseResponse(response);
}

bool OpenAIProvider::testConnection(const String &apiKey, const String &endpoint)
{
    if (apiKey.length() == 0)
    {
        LOG_ERROR("OPENAI", "Cannot test connection: API key not provided");
        return false;
    }

    // Use test endpoint (GET /v1/models) for connection verification
    String testEndpoint = endpoint.length() > 0 ? endpoint : OPENAI_TEST_ENDPOINT;

    String bearerToken = apiKey;
    if (!bearerToken.startsWith("Bearer "))
    {
        bearerToken = "Bearer " + bearerToken;
    }

    LOG_VERBOSE("OPENAI", "Testing connection to: %s", testEndpoint.c_str());

    String response = fetchFromAPIWithBearer(
        testEndpoint,
        bearerToken,
        "ScribeEvolution/1.0",
        5000 // 5 second timeout
    );

    if (response.length() == 0)
    {
        LOG_ERROR("OPENAI", "Connection test failed: no response");
        return false;
    }

    // Parse response to verify it's valid JSON with expected structure
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        LOG_ERROR("OPENAI", "Connection test failed: invalid JSON response");
        return false;
    }

    // Check if response contains "data" array (models list)
    if (!doc["data"].is<JsonArray>())
    {
        LOG_ERROR("OPENAI", "Connection test failed: unexpected response format");
        return false;
    }

    LOG_VERBOSE("OPENAI", "Connection test successful");
    return true;
}

const char **OpenAIProvider::getSupportedModels(int &count) const
{
    count = OPENAI_MODEL_COUNT;
    return OPENAI_MODELS;
}

const char *OpenAIProvider::getDefaultModel() const
{
    return "gpt-4o-mini";
}

String OpenAIProvider::parseResponse(const String &response)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        LOG_ERROR("OPENAI", "Response parsing failed: %s", error.c_str());
        return "";
    }

    // Validate response structure
    if (!doc["choices"].is<JsonVariant>() || doc["choices"].size() == 0)
    {
        LOG_ERROR("OPENAI", "Response missing choices array");
        LOG_ERROR("OPENAI", "Response was: %s", response.c_str());
        return "";
    }

    JsonObject firstChoice = doc["choices"][0];
    if (!firstChoice["message"].is<JsonVariant>() ||
        !firstChoice["message"]["content"].is<JsonVariant>())
    {
        LOG_ERROR("OPENAI", "Response missing message.content field");
        LOG_ERROR("OPENAI", "Response was: %s", response.c_str());
        return "";
    }

    String content = firstChoice["message"]["content"].as<String>();
    content.trim();

    if (content.length() == 0)
    {
        LOG_ERROR("OPENAI", "OpenAI returned empty content");
        return "";
    }

    LOG_VERBOSE("OPENAI", "Extracted content: %s", content.c_str());
    return content;
}
