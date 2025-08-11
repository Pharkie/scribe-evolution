/**
 * @file web_server.h
 * @brief Core web server setup and routing for Scribe ESP32-C3 Thermal Printer
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

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <LittleFS.h>

/**
 * @brief Initialize web server routes and handlers
 * @param maxChars Maximum characters allowed in a message
 */
void setupWebServerRoutes(int maxChars);

/**
 * @brief Helper function to recursively list directory contents for debugging
 * @param dir Directory file handle
 * @param output String to append directory listing to
 * @param level Indentation level for nested directories
 */
void listDirectory(File dir, String &output, int level);

#endif // WEB_SERVER_H
