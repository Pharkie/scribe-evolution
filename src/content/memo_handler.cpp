/**
 * @file memo_handler.cpp
 * @brief Implementation of memo processing and placeholder expansion
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "memo_handler.h"
#include "../utils/time_utils.h"
#include "../core/logging.h"
#include "../core/network.h"
#include "../core/config_utils.h"
#include <WiFi.h>
#include <ESPmDNS.h>

String processMemoPlaceholders(const String &memoText)
{
    String result = memoText;
    
    // Process placeholders in order - replace only the first occurrence each time
    int startPos = 0;
    while ((startPos = result.indexOf('[', startPos)) != -1)
    {
        int endPos = result.indexOf(']', startPos);
        if (endPos == -1)
        {
            break; // No closing bracket found
        }
        
        String placeholder = result.substring(startPos, endPos + 1);
        String expanded = expandPlaceholder(placeholder);
        
        // Replace only this specific occurrence, not all occurrences
        String before = result.substring(0, startPos);
        String after = result.substring(endPos + 1);
        result = before + expanded + after;
        
        startPos = startPos + expanded.length(); // Move past the replacement
    }
    
    return result;
}

String expandPlaceholder(const String &placeholder)
{
    String content = placeholder.substring(1, placeholder.length() - 1); // Remove [ and ]
    content.toLowerCase();
    
    // Simple placeholders
    if (content == "date")
    {
        return getMemoDate();
    }
    else if (content == "time")
    {
        return getMemoTime();
    }
    else if (content == "weekday")
    {
        return getMemoWeekday();
    }
    else if (content == "coin")
    {
        return processCoinPlaceholder();
    }
    else if (content == "uptime")
    {
        return getDeviceUptime();
    }
    else if (content == "ip")
    {
        return getDeviceIP();
    }
    else if (content == "mdns")
    {
        return getDeviceMDNS();
    }
    // Complex placeholders
    else if (content.startsWith("pick:"))
    {
        String options = content.substring(5); // Remove "pick:"
        return processPickPlaceholder(options);
    }
    else if (content.startsWith("dice:"))
    {
        String sidesStr = content.substring(5); // Remove "dice:"
        int sides = sidesStr.toInt();
        if (sides <= 0) sides = 6; // Default to 6 if invalid
        return processDicePlaceholder(sides);
    }
    else if (content == "dice") // Default dice
    {
        return processDicePlaceholder(6);
    }
    
    // Unknown placeholder - return as-is
    return placeholder;
}

String processPickPlaceholder(const String &options)
{
    if (options.length() == 0)
    {
        return "???";
    }
    
    // Split by pipe character
    int count = 1;
    for (int i = 0; i < options.length(); i++)
    {
        if (options.charAt(i) == '|') count++;
    }
    
    // Get random index
    int randomIndex = random(0, count);
    
    // Find the selected option
    int currentIndex = 0;
    int startPos = 0;
    for (int i = 0; i <= options.length(); i++)
    {
        if (i == options.length() || options.charAt(i) == '|')
        {
            if (currentIndex == randomIndex)
            {
                return options.substring(startPos, i);
            }
            currentIndex++;
            startPos = i + 1;
        }
    }
    
    return "???"; // Fallback
}

String processDicePlaceholder(int sides)
{
    if (sides <= 0) sides = 6;
    return String(random(1, sides + 1));
}

String processCoinPlaceholder()
{
    return random(0, 2) == 0 ? "Heads" : "Tails";
}

String getDeviceIP()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return WiFi.localIP().toString();
    }
    return "Not Connected";
}

String getDeviceMDNS()
{
    return String(getMdnsHostname()) + ".local";
}