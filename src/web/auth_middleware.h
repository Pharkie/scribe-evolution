/**
 * @file auth_middleware.h
 * @brief Session-based authentication middleware for web API endpoints
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <IPAddress.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../config/config.h"

/**
 * @brief Session data structure for tracking authenticated users
 */
struct Session {
    char token[sessionTokenLength + 1];  // Session token + null terminator
    char csrf[sessionTokenLength + 1];   // CSRF token + null terminator
    IPAddress clientIP;                  // Client IP address for additional security
    unsigned long lastActivity;          // Timestamp of last API request
    bool active;                         // Whether this session slot is in use
};

/**
 * @brief Initialize the authentication system
 * Sets up session storage and generates cryptographic material
 */
void initAuthSystem();

/**
 * @brief Create a new session for a client
 * @param clientIP IP address of the client
 * @return String containing the session token, or empty string on failure
 */
String createSession(const IPAddress& clientIP);

/**
 * @brief Validate a session token
 * @param token Session token to validate
 * @param clientIP IP address of the requesting client
 * @return true if session is valid and active, false otherwise
 */
bool validateSession(const String& token, const IPAddress& clientIP);

/**
 * @brief Refresh a session's activity timestamp
 * @param token Session token to refresh
 */
void refreshSession(const String& token);

/**
 * @brief Clean up expired sessions
 * Called periodically to free up session slots
 */
void cleanupExpiredSessions();

/**
 * @brief Check if authentication is required for a given path
 * @param path URL path to check
 * @return true if path requires authentication, false if public
 */
bool requiresAuthentication(const String& path);

/**
 * @brief Authentication middleware wrapper for API handlers
 * @param request AsyncWebServerRequest object
 * @param handler Function to call if authenticated
 */
void authenticatedHandler(AsyncWebServerRequest* request, 
                         std::function<void(AsyncWebServerRequest*)> handler);

/**
 * @brief Get formatted session cookie value
 * @param sessionToken Token to set in cookie
 * @return Formatted cookie value string
 */
String getSessionCookieValue(const String& sessionToken);
String getCsrfCookieValue(const String& csrfToken);

/**
 * @brief Set session cookie on HTTP response
 * @param request AsyncWebServerRequest object
 * @param sessionToken Token to set in cookie
 */
void setSessionCookie(AsyncWebServerRequest* request, const String& sessionToken);

/**
 * @brief Extract session token from cookie
 * @param request AsyncWebServerRequest object
 * @return Session token string, or empty if not found
 */
String getSessionToken(AsyncWebServerRequest* request);
String getCookieValue(AsyncWebServerRequest* request, const String& cookieName);

/**
 * @brief Get authentication statistics for diagnostics
 * @param activeSessionCount Reference to store number of active sessions
 * @param totalSessionsCreated Reference to store total sessions created
 */
void getAuthStats(int& activeSessionCount, unsigned long& totalSessionsCreated);

/**
 * @brief Force cleanup of all sessions (e.g., on device restart)
 */
void clearAllSessions();

/**
 * @brief Generate a cryptographically secure random session token
 * @return 32-character hexadecimal token string
 */
String generateSecureToken();

/**
 * @brief Constant-time string comparison to prevent timing attacks
 * @param a First string to compare
 * @param b Second string to compare
 * @return true if strings are equal, false otherwise
 */
bool constantTimeCompare(const String& a, const String& b);

/**
 * @brief Validate CSRF token for a session and client IP
 */
bool validateCsrf(const String& sessionToken, const String& csrfToken, const IPAddress& clientIP);

/**
 * @brief Retrieve CSRF token for a given session+IP
 */
String getCsrfForSession(const String& sessionToken, const IPAddress& clientIP);

#endif // AUTH_MIDDLEWARE_H
