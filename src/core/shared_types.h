/**
 * @file shared_types.h
 * @brief Shared data structures and global variables for Scribe ESP32-C3 Thermal Printer
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

#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#include <Arduino.h>
#include <vector>

/**
 * @brief Structure to hold message data for printing
 */
struct Message
{
    String message;          ///< The content to print
    String timestamp;        ///< When the message was created
    bool shouldPrintLocally; ///< Whether this message should be printed locally
};

/**
 * @brief Structure to hold discovered printer information
 */
struct DiscoveredPrinter
{
    String printerId;
    String name;
    String firmwareVersion;
    String mdns;
    String ipAddress;
    String status;
    String lastPowerOn;
    String timezone;
    unsigned long lastSeen;
};

/// Global variable to store current message for printing
extern Message currentMessage;

#endif // SHARED_TYPES_H
