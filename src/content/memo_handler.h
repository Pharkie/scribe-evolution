/**
 * @file memo_handler.h
 * @brief Memo management and placeholder expansion for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef MEMO_HANDLER_H
#define MEMO_HANDLER_H

#include <Arduino.h>

/**
 * @brief Process memo text with placeholder expansion
 * @param memoText The raw memo text with placeholders
 * @return String with all placeholders expanded
 */
String processMemoPlaceholders(const String &memoText);

/**
 * @brief Expand a specific placeholder type
 * @param placeholder The placeholder string to expand (e.g., "[date]", "[pick:a|b|c]")
 * @return Expanded string value
 */
String expandPlaceholder(const String &placeholder);

/**
 * @brief Process pick placeholder (random selection from options)
 * @param options The options string (e.g., "opt1|opt2|opt3")
 * @return Random selected option
 */
String processPickPlaceholder(const String &options);

/**
 * @brief Process dice placeholder (random number)
 * @param sides Number of sides (default 6)
 * @return Random number as string
 */
String processDicePlaceholder(int sides = 6);

/**
 * @brief Process coin placeholder
 * @return "Heads" or "Tails"
 */
String processCoinPlaceholder();

/**
 * @brief Get device IP address for memo placeholders
 * @return IP address as string
 */
String getDeviceIP();

/**
 * @brief Get device mDNS hostname for memo placeholders
 * @return mDNS hostname as string
 */
String getDeviceMDNS();

#endif // MEMO_HANDLER_H