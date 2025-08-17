/**
 * @file content_generators.cpp
 * @brief Implementation of content generation functions for entertainment endpoints
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32-C3 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 * Based on the original Project Scribe by UrbanCircles.
 */

#include "content_generators.h"
#include "../utils/api_client.h"
#include "../utils/time_utils.h"
#include "../core/config.h"
#include "../core/config_loader.h"
#include "../core/logging.h"
#include "unbidden_ink.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

String generateRiddleContent()
{
    // LittleFS is already mounted in main.cpp, no need to call begin() again

    // Open the riddles.ndjson file
    File file = LittleFS.open("/resources/riddles.ndjson", "r");
    if (!file)
    {
        return "Failed to open riddles file";
    }

    // Pick a random riddle
    int target = random(0, totalRiddles);
    int current = 0;
    String riddleText = "";
    String riddleAnswer = "";

    while (file.available() && current <= target)
    {
        String line = file.readStringUntil('\n');
        line.trim();

        if (current == target)
        {
            DynamicJsonDocument doc(jsonDocumentSize);
            DeserializationError error = deserializeJson(doc, line);

            if (!error && doc.containsKey("riddle"))
            {
                String extracted = doc["riddle"].as<String>();
                if (extracted.length() > 0)
                {
                    riddleText = extracted;
                }

                // Also extract the answer if available
                if (doc.containsKey("answer"))
                {
                    String extractedAnswer = doc["answer"].as<String>();
                    if (extractedAnswer.length() > 0)
                    {
                        riddleAnswer = extractedAnswer;
                    }
                }
            }
            break;
        }
        current++;
    }

    file.close();

    // Return empty string if no riddle was found
    if (riddleText.length() == 0 || riddleAnswer.length() == 0)
    {
        LOG_ERROR("RIDDLE", "Failed to load riddle from file");
        return "";
    }

    // Return raw content without RIDDLE header - header will be added at print time
    String fullContent = "#" + String(target + 1) + "\n\n" + riddleText + "\n\n\n\n\n\n";
    fullContent += "Answer: " + reverseString(riddleAnswer);

    return fullContent;
}

String generateJokeContent(int timeoutMs)
{
    // Try to fetch from API
    String response = fetchFromAPI(jokeAPI, apiUserAgent, timeoutMs);

    if (response.length() > 0)
    {
        DynamicJsonDocument doc(jsonDocumentSize);
        DeserializationError error = deserializeJson(doc, response);

        if (!error && doc.containsKey("joke"))
        {
            String apiJoke = doc["joke"].as<String>();
            apiJoke.trim();
            if (apiJoke.length() > minJokeLength) // Ensure it's a real joke, not empty
            {
                // Return raw joke content without JOKE header - header will be added at print time
                return apiJoke;
            }
        }
    }

    LOG_ERROR("JOKE", "Failed to fetch joke from API (timeout: %dms)", timeoutMs);
    return ""; // Return empty string to indicate failure
}

String generateQuoteContent(int timeoutMs)
{
    // Try to fetch from API
    String response = fetchFromAPI(quoteAPI, apiUserAgent, timeoutMs);

    if (response.length() > 0)
    {
        // Parse JSON response (expecting array format)
        DynamicJsonDocument doc(largeJsonDocumentSize);
        DeserializationError error = deserializeJson(doc, response);

        if (!error && doc.is<JsonArray>() && doc.size() > 0)
        {
            JsonObject quoteObj = doc[0];
            if (quoteObj.containsKey("q") && quoteObj.containsKey("a"))
            {
                String quoteText = quoteObj["q"].as<String>();
                String author = quoteObj["a"].as<String>();

                quoteText.trim();
                author.trim();

                if (quoteText.length() > 0 && author.length() > 0)
                {
                    String quote = "\"" + quoteText + "\"\n– " + author;
                    // Return raw quote content without QUOTE header - header will be added at print time
                    return quote;
                }
            }
        }
    }

    LOG_ERROR("QUOTE", "Failed to fetch quote from API (timeout: %dms)", timeoutMs);
    return ""; // Return empty string to indicate failure
}

String generateQuizContent(int timeoutMs)
{
    // Try to fetch from API
    String response = fetchFromAPI(triviaAPI, apiUserAgent, timeoutMs);

    if (response.length() > 0)
    {
        DynamicJsonDocument doc(largeJsonDocumentSize);
        DeserializationError error = deserializeJson(doc, response);

        if (!error && doc.is<JsonArray>() && doc.size() > 0)
        {
            JsonObject questionObj = doc[0];
            if (questionObj.containsKey("question") &&
                questionObj.containsKey("correctAnswer") &&
                questionObj.containsKey("incorrectAnswers"))
            {
                String question = questionObj["question"].as<String>();
                String correctAnswer = questionObj["correctAnswer"].as<String>();
                JsonArray incorrectAnswers = questionObj["incorrectAnswers"];

                question.trim();
                correctAnswer.trim();

                if (question.length() > 0 && correctAnswer.length() > 0 && incorrectAnswers.size() >= 3)
                {
                    // Randomize the position of the correct answer (A, B, C, or D)
                    int correctPosition = random(0, 4);
                    String options[4];
                    String positionLabels[4] = {"A", "B", "C", "D"};

                    // Fill with incorrect answers first
                    int incorrectIndex = 0;
                    for (int i = 0; i < 4; i++)
                    {
                        if (i == correctPosition)
                        {
                            options[i] = correctAnswer;
                        }
                        else
                        {
                            options[i] = incorrectAnswers[incorrectIndex].as<String>();
                            incorrectIndex++;
                        }
                    }

                    // Return raw quiz content without QUIZ header - header will be added at print time
                    String quiz = question + "\n";
                    quiz += "A) " + options[0] + "\n";
                    quiz += "B) " + options[1] + "\n";
                    quiz += "C) " + options[2] + "\n";
                    quiz += "D) " + options[3] + "\n\n\n\n";
                    quiz += "Answer: " + reverseString(correctAnswer);
                    return quiz;
                }
            }
        }
    }

    LOG_ERROR("QUIZ", "Failed to fetch quiz from API (timeout: %dms)", timeoutMs);
    return ""; // Return empty string to indicate failure
}

String generateUnbiddenInkContent(const String &customPrompt)
{
    // Get token, endpoint, and prompt from config and dynamic settings
    const RuntimeConfig &config = getRuntimeConfig();
    String apiToken = config.chatgptApiToken;
    String apiEndpoint = chatgptApiEndpoint;

    // Use custom prompt if provided, otherwise use the saved prompt
    String prompt = customPrompt.length() > 0 ? customPrompt : getUnbiddenInkPrompt();

    // Build Bearer token with automatic prefix
    String bearerToken = "Bearer " + apiToken;

    LOG_VERBOSE("UNBIDDENINK", "Calling ChatGPT API: %s", apiEndpoint.c_str());
    LOG_VERBOSE("UNBIDDENINK", "Using prompt: %s", prompt.c_str());

    // Build JSON payload for OpenAI ChatGPT API
    DynamicJsonDocument payloadDoc(largeJsonDocumentSize);
    payloadDoc["model"] = "gpt-4o-mini";
    payloadDoc["max_tokens"] = 150;
    payloadDoc["temperature"] = 0.7;

    JsonArray messages = payloadDoc.createNestedArray("messages");
    JsonObject userMessage = messages.createNestedObject();
    userMessage["role"] = "user";
    userMessage["content"] = prompt;

    String jsonPayload;
    serializeJson(payloadDoc, jsonPayload);

    // POST to OpenAI ChatGPT API with Bearer token
    String response = postToAPIWithBearer(apiEndpoint, bearerToken, jsonPayload, apiUserAgent);

    if (response.length() > 0)
    {
        LOG_VERBOSE("UNBIDDENINK", "API response received: %s", response.c_str());

        // Parse OpenAI ChatGPT JSON response
        DynamicJsonDocument doc(largeJsonDocumentSize);
        DeserializationError error = deserializeJson(doc, response);

        if (!error && doc.containsKey("choices") && doc["choices"].size() > 0)
        {
            JsonObject firstChoice = doc["choices"][0];
            if (firstChoice.containsKey("message") && firstChoice["message"].containsKey("content"))
            {
                String apiMessage = firstChoice["message"]["content"].as<String>();
                apiMessage.trim();
                if (apiMessage.length() > 0)
                {
                    LOG_VERBOSE("UNBIDDENINK", "Using ChatGPT content: %s", apiMessage.c_str());
                    return apiMessage; // Return raw content, header will be added by formatContentWithHeader()
                }
                else
                {
                    LOG_ERROR("UNBIDDENINK", "ChatGPT returned empty content");
                    return ""; // Return empty string to indicate failure
                }
            }
            else
            {
                LOG_ERROR("UNBIDDENINK", "ChatGPT response missing message.content field");
                return ""; // Return empty string to indicate failure
            }
        }
        else
        {
            LOG_ERROR("UNBIDDENINK", "ChatGPT response parsing failed or no choices found");
            // Log the actual response for debugging
            LOG_ERROR("UNBIDDENINK", "Response was: %s", response.c_str());
            return ""; // Return empty string to indicate failure
        }
    }
    else
    {
        LOG_ERROR("UNBIDDENINK", "No response from ChatGPT API");
        return ""; // Return empty string to indicate failure
    }
}

String generatePokeContent()
{
    // Poke has no content, just the action header
    return "";
}

String generateNewsContent(int timeoutMs)
{
    LOG_VERBOSE("NEWS", "Fetching news from BBC RSS feed");

    // BBC RSS feed URL from config
    const String url = newsAPI;

    // Fetch the RSS XML content
    String response = fetchFromAPI(url, apiUserAgent, timeoutMs);

    if (response.length() == 0)
    {
        LOG_ERROR("NEWS", "Failed to fetch news from BBC RSS feed");
        return ""; // Return empty string to indicate failure
    }

    // Parse the RSS XML response
    String newsContent = "";
    int itemCount = 0;
    const int maxItems = 5; // Limit to 5 news items to avoid overwhelming the display

    // Simple XML parsing - find <item> blocks
    int itemStart = 0;
    while (itemCount < maxItems)
    {
        // Find next <item> tag
        itemStart = response.indexOf("<item>", itemStart);
        if (itemStart == -1)
            break;

        // Find the end of this item
        int itemEnd = response.indexOf("</item>", itemStart);
        if (itemEnd == -1)
            break;

        // Extract item content
        String itemXML = response.substring(itemStart, itemEnd + 7);

        // Extract title
        int titleStart = itemXML.indexOf("<title>");
        int titleEnd = itemXML.indexOf("</title>");
        String title = "";
        if (titleStart != -1 && titleEnd != -1)
        {
            title = itemXML.substring(titleStart + 7, titleEnd);
            // Clean up CDATA if present
            title.replace("<![CDATA[", "");
            title.replace("]]>", "");
            title.trim();
        }

        // Extract pub date
        int pubDateStart = itemXML.indexOf("<pubDate>");
        int pubDateEnd = itemXML.indexOf("</pubDate>");
        String pubDate = "";
        if (pubDateStart != -1 && pubDateEnd != -1)
        {
            String rawPubDate = itemXML.substring(pubDateStart + 9, pubDateEnd);
            rawPubDate.trim();

            // Parse RFC 2822 format using time utils
            pubDate = formatRFC2822Date(rawPubDate);
        }

        // Add to news content if we have a title
        if (title.length() > 0)
        {
            if (newsContent.length() > 0)
            {
                newsContent += "\n\n";
            }

            if (pubDate.length() > 0)
            {
                newsContent += pubDate + "\n";
            }
            newsContent += title;

            itemCount++;
        }

        // Move to next item
        itemStart = itemEnd + 7;
    }

    if (newsContent.length() == 0)
    {
        LOG_ERROR("NEWS", "No news items found in RSS feed");
        return "";
    }

    LOG_VERBOSE("NEWS", "Generated news content with %d items", itemCount);
    return newsContent;
}
