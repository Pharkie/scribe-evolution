/**
 * @file content_generators.h
 * @brief Content generation functions for entertainment endpoints
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
 */

#ifndef CONTENT_GENERATORS_H
#define CONTENT_GENERATORS_H

#include <Arduino.h>

/**
 * @brief Content generation functions for entertainment endpoints
 *
 * This module handles the generation of content for riddles, jokes, quotes,
 * and quiz questions. Content is sourced from local files and external APIs.
 * Functions return raw content without headers - headers are added at print time.
 */

/**
 * @brief Generate riddle content from local NDJSON file
 * @return String containing raw riddle with answer (no header)
 */
String generateRiddleContent();

/**
 * @brief Generate joke content from external API with fallback
 * @param timeoutMs Custom timeout in milliseconds (default: 5000)
 * @return String containing raw joke (no header)
 */
String generateJokeContent(int timeoutMs = 5000);

/**
 * @brief Generate quote content from external API with fallback
 * @param timeoutMs Custom timeout in milliseconds (default: 5000)
 * @return String containing raw quote with attribution (no header)
 */
String generateQuoteContent(int timeoutMs = 5000);

/**
 * @brief Generate quiz content from external API with fallback
 * @param timeoutMs Custom timeout in milliseconds (default: 5000)
 * @return String containing raw multiple choice question with answer (no header)
 */
String generateQuizContent(int timeoutMs = 5000);

/**
 * @brief Generate AI content from Unbidden Ink API
 * @return String containing raw Unbidden Ink content (no header)
 */
String generateUnbiddenInkContent(const String &customPrompt = "");

/**
 * @brief Generate poke content (empty content, just for notification)
 * @return Empty string (poke has no content, just the action header)
 */
String generatePokeContent();

/**
 * @brief Generate news content from BBC RSS feed
 * @param timeoutMs Custom timeout in milliseconds (default: 10000)
 * @return String containing raw news headlines with dates (no header)
 */
String generateNewsContent(int timeoutMs = 10000);

/**
 * @brief Generate AP (Access Point) connection details for startup printing
 * @return String containing formatted AP connection instructions (no header)
 */
String generateAPDetailsContent();

#endif // CONTENT_GENERATORS_H
