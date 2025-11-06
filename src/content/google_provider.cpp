/**
 * @file google_provider.cpp
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

#include "google_provider.h"
#include <utils/api_client.h>
#include <core/logging.h>
#include <ArduinoJson.h>

// Supported Google models
static const char *GOOGLE_MODELS[] = {
    "gemini-2.0-flash-exp",
    "gemini-1.5-flash",
    "gemini-1.5-pro"
};
static const int GOOGLE_MODEL_COUNT = sizeof(GOOGLE_MODELS) / sizeof(GOOGLE_MODELS[0]);

// Default API endpoints
static const char *GOOGLE_API_BASE = "https://generativelanguage.googleapis.com/v1beta/models/";

String GoogleProvider::generateContent(const String &prompt, const AIProviderConfig &config)
{
    // Validate configuration
    if (config.apiKey.length() == 0)
    {
        LOG_ERROR("GOOGLE", "API key not configured");
        return "";
    }

    // Use provided model or default
    String model = config.model.length() > 0 ? config.model : getDefaultModel();

    // Build endpoint URL with model and API key
    String endpoint = buildEndpointURL(model, config.apiKey, config.endpoint);

    LOG_VERBOSE("GOOGLE", "Calling Google Gemini API: %s", endpoint.c_str());
    LOG_VERBOSE("GOOGLE", "Using model: %s", model.c_str());
    LOG_VERBOSE("GOOGLE", "Using prompt: %s", prompt.c_str());

    // Build JSON payload for Google Gemini API
    JsonDocument payloadDoc;

    // Contents array with single user message
    JsonArray contents = payloadDoc["contents"].to<JsonArray>();
    JsonObject content = contents.add<JsonObject>();
    JsonArray parts = content["parts"].to<JsonArray>();
    JsonObject part = parts.add<JsonObject>();
    part["text"] = prompt;

    // Generation configuration
    JsonObject generationConfig = payloadDoc["generationConfig"].to<JsonObject>();
    generationConfig["temperature"] = config.temperature;
    generationConfig["maxOutputTokens"] = config.maxTokens;

    String jsonPayload;
    serializeJson(payloadDoc, jsonPayload);

    // POST to Google Gemini API (no custom headers needed, API key in URL)
    String response = postToAPIWithBearer(
        endpoint,
        "", // No bearer token needed (key in URL)
        jsonPayload,
        "ScribeEvolution/1.0",
        config.timeoutMs
    );

    if (response.length() == 0)
    {
        LOG_ERROR("GOOGLE", "No response from Google Gemini API");
        return "";
    }

    LOG_VERBOSE("GOOGLE", "API response received: %s", response.c_str());

    // Parse response and return content
    return parseResponse(response);
}

bool GoogleProvider::testConnection(const String &apiKey, const String &endpoint)
{
    if (apiKey.length() == 0)
    {
        LOG_ERROR("GOOGLE", "Cannot test connection: API key not provided");
        return false;
    }

    // Build test endpoint URL
    String testEndpoint = buildEndpointURL(getDefaultModel(), apiKey, endpoint);

    LOG_VERBOSE("GOOGLE", "Testing connection to: %s", testEndpoint.c_str());

    // Build minimal test payload
    JsonDocument payloadDoc;

    JsonArray contents = payloadDoc["contents"].to<JsonArray>();
    JsonObject content = contents.add<JsonObject>();
    JsonArray parts = content["parts"].to<JsonArray>();
    JsonObject part = parts.add<JsonObject>();
    part["text"] = "test";

    JsonObject generationConfig = payloadDoc["generationConfig"].to<JsonObject>();
    generationConfig["maxOutputTokens"] = 10; // Minimal tokens for test

    String jsonPayload;
    serializeJson(payloadDoc, jsonPayload);

    String response = postToAPIWithBearer(
        testEndpoint,
        "", // No bearer token needed
        jsonPayload,
        "ScribeEvolution/1.0",
        5000 // 5 second timeout
    );

    if (response.length() == 0)
    {
        LOG_ERROR("GOOGLE", "Connection test failed: no response");
        return false;
    }

    // Parse response to verify it's valid JSON with expected structure
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        LOG_ERROR("GOOGLE", "Connection test failed: invalid JSON response");
        return false;
    }

    // Check if response contains "candidates" array (valid response)
    if (!doc["candidates"].is<JsonArray>())
    {
        LOG_ERROR("GOOGLE", "Connection test failed: unexpected response format");
        return false;
    }

    LOG_VERBOSE("GOOGLE", "Connection test successful");
    return true;
}

const char **GoogleProvider::getSupportedModels(int &count) const
{
    count = GOOGLE_MODEL_COUNT;
    return GOOGLE_MODELS;
}

const char *GoogleProvider::getDefaultModel() const
{
    return "gemini-1.5-flash";
}

String GoogleProvider::buildEndpointURL(const String &model, const String &apiKey, const String &baseEndpoint)
{
    // Use custom base endpoint if provided, otherwise use default
    String base = baseEndpoint.length() > 0 ? baseEndpoint : GOOGLE_API_BASE;

    // Remove trailing slash if present
    if (base.endsWith("/"))
    {
        base = base.substring(0, base.length() - 1);
    }

    // Build URL: base/model:generateContent?key=apiKey
    String url = base + "/" + model + ":generateContent?key=" + apiKey;

    return url;
}

String GoogleProvider::parseResponse(const String &response)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        LOG_ERROR("GOOGLE", "Response parsing failed: %s", error.c_str());
        return "";
    }

    // Validate response structure
    if (!doc["candidates"].is<JsonArray>() || doc["candidates"].size() == 0)
    {
        LOG_ERROR("GOOGLE", "Response missing candidates array");
        LOG_ERROR("GOOGLE", "Response was: %s", response.c_str());
        return "";
    }

    JsonArray candidates = doc["candidates"];
    JsonObject firstCandidate = candidates[0];

    // Navigate to content.parts[0].text
    if (!firstCandidate["content"].is<JsonVariant>() ||
        !firstCandidate["content"]["parts"].is<JsonArray>() ||
        firstCandidate["content"]["parts"].size() == 0)
    {
        LOG_ERROR("GOOGLE", "Response missing content.parts array");
        LOG_ERROR("GOOGLE", "Response was: %s", response.c_str());
        return "";
    }

    JsonArray parts = firstCandidate["content"]["parts"];
    JsonObject firstPart = parts[0];

    if (!firstPart["text"].is<JsonVariant>())
    {
        LOG_ERROR("GOOGLE", "Response missing text field");
        LOG_ERROR("GOOGLE", "Response was: %s", response.c_str());
        return "";
    }

    String content = firstPart["text"].as<String>();
    content.trim();

    if (content.length() == 0)
    {
        LOG_ERROR("GOOGLE", "Google returned empty content");
        return "";
    }

    LOG_VERBOSE("GOOGLE", "Extracted content: %s", content.c_str());
    return content;
}
