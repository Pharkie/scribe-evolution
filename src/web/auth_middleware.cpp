/**
 * @file auth_middleware.cpp
 * @brief Session-based authentication middleware implementation
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "auth_middleware.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/network.h"
#include <esp_random.h>
#include <functional>

// Global session storage
static Session sessions[maxConcurrentSessions];
static unsigned long totalSessionsCreated = 0;
static int nextSessionSlot = 0;

// Statistics
static unsigned long lastCleanupTime = 0;
static const unsigned long cleanupIntervalMs = 300000; // 5 minutes

void initAuthSystem() {
    LOG_NOTICE("AUTH", "Initializing session authentication system");
    
    // Initialize all session slots
    for (int i = 0; i < maxConcurrentSessions; i++) {
        sessions[i].token[0] = '\0';
        sessions[i].clientIP = IPAddress(0, 0, 0, 0);
        sessions[i].lastActivity = 0;
        sessions[i].active = false;
    }
    
    totalSessionsCreated = 0;
    nextSessionSlot = 0;
    lastCleanupTime = millis();
    
    LOG_NOTICE("AUTH", "Auth system initialized - max %d concurrent sessions, %d hour timeout", 
               maxConcurrentSessions, sessionTimeoutMs / 3600000);
}

String generateSecureToken() {
    String token = "";
    token.reserve(sessionTokenLength + 1);
    
    // Generate random bytes using ESP32 hardware RNG
    for (int i = 0; i < sessionTokenLength / 2; i++) {
        uint32_t randomValue = esp_random();
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", randomValue & 0xFF);
        token += hex;
    }
    
    return token;
}

bool constantTimeCompare(const String& a, const String& b) {
    if (a.length() != b.length()) {
        return false;
    }
    
    int result = 0;
    for (size_t i = 0; i < a.length(); i++) {
        result |= (a[i] ^ b[i]);
    }
    
    return result == 0;
}

String createSession(const IPAddress& clientIP) {
    cleanupExpiredSessions(); // Clean up before creating new session
    
    // Find an available slot (circular buffer)
    int startSlot = nextSessionSlot;
    do {
        if (!sessions[nextSessionSlot].active) {
            break; // Found available slot
        }
        nextSessionSlot = (nextSessionSlot + 1) % maxConcurrentSessions;
    } while (nextSessionSlot != startSlot);
    
    // If no slots available, use the current one anyway (LRU replacement)
    Session& session = sessions[nextSessionSlot];
    
    // Generate new session
    String token = generateSecureToken();
    if (token.length() != sessionTokenLength) {
        LOG_ERROR("AUTH", "Failed to generate session token");
        return "";
    }
    
    strncpy(session.token, token.c_str(), sizeof(session.token) - 1);
    session.token[sizeof(session.token) - 1] = '\0';
    session.clientIP = clientIP;
    session.lastActivity = millis();
    session.active = true;
    
    totalSessionsCreated++;
    nextSessionSlot = (nextSessionSlot + 1) % maxConcurrentSessions;
    
    LOG_VERBOSE("AUTH", "Created session for IP %s (slot %d)", 
                clientIP.toString().c_str(), nextSessionSlot - 1);
    
    return token;
}

bool validateSession(const String& token, const IPAddress& clientIP) {
    if (token.length() != sessionTokenLength) {
        return false;
    }
    
    unsigned long currentTime = millis();
    
    // Check all session slots
    for (int i = 0; i < maxConcurrentSessions; i++) {
        if (!sessions[i].active) {
            continue;
        }
        
        // Check if session has expired
        if (currentTime - sessions[i].lastActivity > sessionTimeoutMs) {
            sessions[i].active = false;
            LOG_VERBOSE("AUTH", "Session expired for IP %s", sessions[i].clientIP.toString().c_str());
            continue;
        }
        
        // Use constant-time comparison to prevent timing attacks
        if (constantTimeCompare(token, String(sessions[i].token)) && 
            sessions[i].clientIP == clientIP) {
            return true;
        }
    }
    
    return false;
}

void refreshSession(const String& token) {
    if (token.length() != sessionTokenLength) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    for (int i = 0; i < maxConcurrentSessions; i++) {
        if (sessions[i].active && constantTimeCompare(token, String(sessions[i].token))) {
            sessions[i].lastActivity = currentTime;
            LOG_VERBOSE("AUTH", "Refreshed session for IP %s", 
                       sessions[i].clientIP.toString().c_str());
            break;
        }
    }
}

void cleanupExpiredSessions() {
    unsigned long currentTime = millis();
    
    // Only run cleanup every 5 minutes to avoid overhead
    if (currentTime - lastCleanupTime < cleanupIntervalMs) {
        return;
    }
    
    int cleanedUp = 0;
    for (int i = 0; i < maxConcurrentSessions; i++) {
        if (sessions[i].active && 
            (currentTime - sessions[i].lastActivity > sessionTimeoutMs)) {
            sessions[i].active = false;
            sessions[i].token[0] = '\0';
            cleanedUp++;
        }
    }
    
    lastCleanupTime = currentTime;
    
    if (cleanedUp > 0) {
        LOG_VERBOSE("AUTH", "Cleaned up %d expired sessions", cleanedUp);
    }
}

bool requiresAuthentication(const String& path) {
    // Check if we're in AP mode - no auth required during setup
    if (isAPMode()) {
        return false;
    }
    
    // Public paths that never require authentication
    const char* publicPaths[] = {
        "/",
        "/index.html",
        "/setup.html",
        "/404.html",
        "/favicon.svg",
        "/css/",
        "/js/",
        "/images/",
        "/api/routes",
        "/ncsi.txt"
    };
    
    const int numPublicPaths = sizeof(publicPaths) / sizeof(publicPaths[0]);
    
    for (int i = 0; i < numPublicPaths; i++) {
        if (path.startsWith(publicPaths[i])) {
            return false;
        }
    }
    
    // Setup endpoints are public
    if (path.startsWith("/api/setup") || 
        path.startsWith("/api/wifi-scan") || 
        path.startsWith("/api/test-wifi") ||
        path.startsWith("/api/timezones")) {
        return false;
    }
    
    // All other API endpoints require authentication
    if (path.startsWith("/api/")) {
        return true;
    }
    
    // Default: no authentication required for static assets
    return false;
}

void authenticatedHandler(AsyncWebServerRequest* request, 
                         std::function<void(AsyncWebServerRequest*)> handler) {
    String path = request->url();
    
    // Check if authentication is required
    if (!requiresAuthentication(path)) {
        handler(request);
        return;
    }
    
    // Extract session token from cookie
    String sessionToken = getSessionToken(request);
    IPAddress clientIP = request->client()->remoteIP();
    
    // Validate session
    if (sessionToken.length() > 0 && validateSession(sessionToken, clientIP)) {
        // Refresh session activity
        refreshSession(sessionToken);
        // Call the original handler
        handler(request);
    } else {
        // Unauthorized - return 401
        LOG_WARNING("AUTH", "Unauthorized access attempt from %s to %s", 
                   clientIP.toString().c_str(), path.c_str());
        
        String errorResponse = "{\"error\":\"Authentication required\",\"code\":401}";
        request->send(401, "application/json", errorResponse);
    }
}

String getSessionCookieValue(const String& sessionToken) {
    if (sessionToken.length() != sessionTokenLength) {
        LOG_ERROR("AUTH", "Invalid session token length for cookie");
        return "";
    }
    
    String cookieValue = String(sessionCookieName) + "=" + sessionToken + "; " + 
                        String(sessionCookieOptions) + "; Max-Age=" + 
                        String(sessionTimeoutMs / 1000);
    
    return cookieValue;
}

void setSessionCookie(AsyncWebServerRequest* request, const String& sessionToken) {
    String cookieValue = getSessionCookieValue(sessionToken);
    if (cookieValue.length() > 0) {
        // This function should be called from handlers that will add the header to their response
        LOG_VERBOSE("AUTH", "Session cookie prepared for %s", 
                   request->client()->remoteIP().toString().c_str());
    }
}

String getSessionToken(AsyncWebServerRequest* request) {
    // Check if the request has a Cookie header
    if (!request->hasHeader("Cookie")) {
        return "";
    }
    
    String cookieHeader = request->header("Cookie");
    String cookieName = String(sessionCookieName) + "=";
    
    int cookieStart = cookieHeader.indexOf(cookieName);
    if (cookieStart == -1) {
        return "";
    }
    
    cookieStart += cookieName.length();
    int cookieEnd = cookieHeader.indexOf(';', cookieStart);
    
    if (cookieEnd == -1) {
        cookieEnd = cookieHeader.length();
    }
    
    String token = cookieHeader.substring(cookieStart, cookieEnd);
    token.trim();
    
    return (token.length() == sessionTokenLength) ? token : "";
}

void getAuthStats(int& activeSessionCount, unsigned long& totalSessionsCreated) {
    activeSessionCount = 0;
    
    for (int i = 0; i < maxConcurrentSessions; i++) {
        if (sessions[i].active) {
            activeSessionCount++;
        }
    }
    
    totalSessionsCreated = ::totalSessionsCreated;
}

void clearAllSessions() {
    LOG_NOTICE("AUTH", "Clearing all active sessions");
    
    for (int i = 0; i < maxConcurrentSessions; i++) {
        sessions[i].active = false;
        sessions[i].token[0] = '\0';
        sessions[i].lastActivity = 0;
    }
    
    nextSessionSlot = 0;
}