#ifndef CHARACTER_MAPPING_H
#define CHARACTER_MAPPING_H

#include <Arduino.h>

/**
 * Character transliteration for thermal printer compatibility
 * This function converts UTF-8 characters, emojis, symbols, and special characters
 * into ASCII equivalents that thermal printers can handle reliably.
 *
 * @param input The input string containing potentially problematic characters
 * @return A cleaned string with all characters mapped to thermal printer safe equivalents
 */
String cleanString(String input);

#endif // CHARACTER_MAPPING_H
