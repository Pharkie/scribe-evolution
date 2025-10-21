/**
 * @file system_constants.h
 * @brief System constants and hardware settings for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file contains system constants, hardware settings, and technical parameters.
 * These rarely need modification and are tuned for ESP32-C3 performance.
 *
 */

#ifndef SYSTEM_CONSTANTS_H
#define SYSTEM_CONSTANTS_H

#include <Arduino.h> // For String type
#include <cstddef>  // For size_t
#include <esp_log.h>

// ----------------------------------------------------------------------------
// Time conversion helpers (express human units, return milliseconds)
// ----------------------------------------------------------------------------
namespace ScribeTime
{
    constexpr unsigned long Seconds(unsigned long s) { return s * 1000UL; }
    constexpr unsigned long Minutes(unsigned long m) { return m * 60UL * 1000UL; }
    constexpr unsigned long Hours(unsigned long h) { return h * 60UL * 60UL * 1000UL; }
}

// ============================================================================
// SYSTEM CONSTANTS - Hardware timings and buffer sizes
// ============================================================================
static const int serialTimeoutMs = 5000;                          // Serial connection timeout (5s)
static const int smallDelayMs = 50;                               // Small delay for CPU/power management
static const int mediumJsonBuffer = 1024;                         // Medium JSON buffer size divisor
static const int defaultMaxRetries = 3;                           // Default max retries for API calls
static const int defaultBaseDelayMs = 1000;                       // Default base delay for backoff (1s)
static const int mqttTestCleanupDelayMs = ScribeTime::Seconds(1); // MQTT test cleanup delay (1s)

// Session management constants
static const int maxConcurrentSessions = 5;                                           // Maximum concurrent user sessions
static const int sessionTokenLength = 32;                                             // Session token length (bytes)
static const unsigned int sessionTimeoutHours = 4;                                    // Session timeout in hours (canonical)
static const unsigned long sessionTimeoutMs = ScribeTime::Hours(sessionTimeoutHours); // Derived ms
static const char *sessionCookieName = "ScribeSession";                               // Session cookie name
static const char *sessionCookieOptions = "HttpOnly; Secure; SameSite=Strict";        // Cookie security options

// MQTT connection and retry settings
static const int mqttMaxConsecutiveFailures = 3;                               // Max failures before cooldown
static const unsigned long mqttReconnectIntervalMs = ScribeTime::Seconds(5);   // Normal reconnect interval (5s)
static const unsigned long mqttFailureCooldownMs = ScribeTime::Minutes(1);     // Cooldown after max failures (60s)
static const unsigned long mqttConnectionTimeoutMs = ScribeTime::Seconds(7);   // Connection timeout (7s)
static const unsigned long mqttTlsHandshakeTimeoutMs = ScribeTime::Seconds(6); // TLS handshake timeout (< watchdog)
static const int mqttBufferSize = 512;                                         // MQTT message buffer size

// Unbidden Ink prompt presets (autoprompts)
static const char *unbiddenInkPromptCreative = "Generate creative, artistic content - poetry, short stories, or imaginative scenarios. Keep it engaging and printable.";
static const char *unbiddenInkPromptWisdom = "Share philosophical insights, life wisdom, or thought-provoking reflections. Keep it meaningful and contemplative.";
static const char *unbiddenInkPromptHumor = "Create funny content - jokes, witty observations, or humorous takes on everyday situations. Keep it light and entertaining.";
static const char *unbiddenInkPromptDoctorWho = "Generate content inspired by Doctor Who - time travel adventures, alien encounters, or sci-fi scenarios with a whimsical tone.";

// Default prompt (use Creative as default)
static const char *defaultUnbiddenInkPrompt = unbiddenInkPromptCreative;

// Default button ACTION configuration (actions, MQTT topics, LED effects)
// Note: Button GPIO pins are defined in board config files (BOARD_BUTTON_PINS)
// This struct only contains user-configurable behavior, NOT hardware wiring
struct ButtonActionConfig
{
    const char *shortAction; // Direct content action type (JOKE, RIDDLE, etc.) - NOT HTTP endpoints
    const char *shortMqttTopic;
    const char *shortLedEffect;
    const char *longAction; // Direct content action type (JOKE, RIDDLE, etc.) - NOT HTTP endpoints
    const char *longMqttTopic;
    const char *longLedEffect;
};

// IMPORTANT: Hardware buttons call internal functions directly, NOT HTTP endpoints
// Actions are content types (JOKE, RIDDLE, QUOTE, etc.) that map to generateXXXContent() functions
// HTTP endpoints (/api/joke, etc.) are for web interface and MQTT only
//
// Note: Button GPIO pins come from board config (BOARD_BUTTON_PINS), actions can be changed in NVS
// Empty MQTT topic means use local printing (no network calls)
static const ButtonActionConfig defaultButtonActions[] = {
    {"JOKE", "", "chase_single", "CHARACTER_TEST", "", "pulse"}, // Button 0: Joke → Character Test
    {"RIDDLE", "", "chase_single", "", "", "pulse"},             // Button 1: Riddle → (no long action)
    {"QUOTE", "", "chase_single", "", "", "pulse"},              // Button 2: Quote → (no long action)
    {"QUIZ", "", "chase_single", "", "", "pulse"}                // Button 3: Quiz → (no long action)
};

// For backwards compatibility during migration
using ButtonConfig = ButtonActionConfig;

// ============================================================================
// BACKEND CONSTANTS - Fixed at compile time, not user-configurable
// ============================================================================

// Unbidden Ink frequency validation limits
constexpr int minUnbiddenInkFrequencyMinutes = 15; // Minimum frequency: 15 minutes
constexpr int maxUnbiddenInkFrequencyMinutes = 480;

// Hardware button settings
static const int numHardwareButtons = sizeof(defaultButtonActions) / sizeof(defaultButtonActions[0]); // Automatically calculated
static const unsigned long buttonDebounceMs = 50;                                         // Debounce time in milliseconds
static const unsigned long buttonLongPressMs = 2000;                                      // Long press threshold in milliseconds
static const bool buttonActiveLow = true;                                                 // true = button pulls to ground, false = button pulls to VCC

// Button rate limiting (separate from debouncing) - made more aggressive for ESP32-C3 stability
static const unsigned long buttonMinInterval = 5000;      // 5 seconds minimum between button presses (increased from 3s)
static const unsigned long buttonMaxPerMinute = 10;       // 10 button presses per minute max (reduced from 20)
static const unsigned long buttonRateLimitWindow = 60000; // 1 minute rate limit window

// Button task safety settings (ESP32-C3 specific)
static const int buttonTaskStackSize = 8192;   // 8KB stack for HTTP operations
static const int buttonTaskPriority = 1;       // Lower than main loop (priority 1)
static const int buttonQueueSize = 10;         // Max queued button actions
static const int buttonActionTimeoutMs = 3000; // 3s timeout for button-triggered HTTP calls (reduced from 5s)

// Network & Time Configuration
static const char *ntpServers[] = {
    "time.cloudflare.com", // Fastest - Cloudflare's global CDN
    "time.google.com",     // Very fast - Google's infrastructure
    "0.pool.ntp.org",      // Traditional reliable pool
    "1.pool.ntp.org"       // Backup pool server
};
static const int ntpServerCount = sizeof(ntpServers) / sizeof(ntpServers[0]);
static const int ntpSyncTimeoutSeconds = 30;    // Maximum wait time for initial NTP sync
static const int ntpSyncIntervalSeconds = 3600; // Re-sync interval (1 hour)

// Logging Configuration
// Logging levels: LOG_LEVEL_VERBOSE, LOG_LEVEL_NOTICE, LOG_LEVEL_WARN, LOG_LEVEL_ERROR
// Note: logLevel, espLogLevel, and enable* flags defined in device_config.h
static const char *mqttLogTopic = "scribe/log";
static const char *logFileName = "/logs/scribe.log";
static const size_t maxLogFileSize = 100000; // 100KB

// External API endpoints
static const char *jokeAPI = "https://icanhazdadjoke.com/";
static const char *quoteAPI = "https://zenquotes.io/api/random";
static const char *triviaAPI = "https://the-trivia-api.com/api/questions?categories=general_knowledge&difficulty=medium&limit=1";
static const char *newsAPI = "https://feeds.bbci.co.uk/news/rss.xml";
static const char *chatgptApiEndpoint = "https://api.openai.com/v1/chat/completions"; // ChatGPT API URL (NEVER exposed to frontend)
static const char *chatgptApiTestEndpoint = "https://api.openai.com/v1/models";       // ChatGPT token test URL (NEVER exposed to frontend)

// BetterStack configuration
static const char *betterStackEndpoint = "https://s1451477.eu-nbg-2.betterstackdata.com/";

// Application Settings
static const int maxCharacters = 1000;      // Max characters per message (single source of truth)
static const int maxPromptCharacters = 500; // Max characters for Unbidden Ink prompts
static const int totalRiddles = 545;        // Total riddles in riddles.ndjson
static const char *apiUserAgent = "Scribe Thermal Printer (https://github.com/Pharkie/scribe)";

// Hardware Configuration - GPIO Defaults (can be overridden in runtime config)
static const int defaultPrinterTxPin = BOARD_PRINTER_TX_PIN; // Board-specific printer TX pin
static const int heatingDots = 10;                           // Heating dots (7-15, lower = less power)
static const int heatingTime = 150;        // Heating time (80-200ms)
static const int heatingInterval = 250;    // Heating interval (200-250ms)

// System Performance Settings
static const unsigned long memCheckIntervalMs = ScribeTime::Minutes(1);    // 60 seconds (memory check frequency)
static const unsigned long reconnectIntervalMs = ScribeTime::Seconds(5);   // 5 seconds (WiFi reconnection interval)
static const unsigned long wifiConnectTimeoutMs = ScribeTime::Seconds(15); // 15 seconds timeout for WiFi connection
static const char *fallbackAPSSID = "Scribe-setup";
static const char *fallbackAPPassword = "scribe123";
static const int statusLEDPin = BOARD_STATUS_LED_PIN; // Board-specific status LED pin
static const int webServerPort = 80;                  // HTTP port for web server
const int watchdogTimeoutSeconds = 8; // Watchdog timeout in seconds

// Printer Discovery Heartbeat
static const unsigned long printerDiscoveryHeartbeatIntervalMs = ScribeTime::Minutes(1); // 1 minute heartbeat interval

// Input Validation Limits
static const unsigned long minRequestIntervalMs = 100;                 // 100ms minimum between requests
static const unsigned long maxRequestsPerMinute = 60;                  // 60 requests per minute
static const unsigned long rateLimitWindowMs = ScribeTime::Minutes(1); // 1 minute rate limit window
static const int maxControlCharPercent = 10;                           // Max control characters as percentage of message length
static const int maxJsonPayloadSize = 8192;                            // 8KB max JSON payload size
static const int maxMqttTopicLength = 128;                             // Max MQTT topic length
static const int maxParameterLength = 1000;                            // Default max parameter length
static const int maxRemoteParameterLength = 100;                       // Max length for remote parameter
static const int maxUriDisplayLength = 200;                            // Max URI length for display (truncated after this)
static const int jsonDocumentSize = 1024;                              // Standard JSON document buffer size
static const int largeJsonDocumentSize = 6144;                         // Large JSON document buffer size (6KB for config with GPIO info and memos)
static const int maxValidationErrors = 10;                             // Max validation errors to store
static const int maxOtherPrinters = 10;                                // Max other printers to track
static const int stringBufferSize = 64;                                // Standard string buffer size
static const int topicBufferSize = 64;                                 // MQTT topic buffer size
static const int maxWifiPasswordLength = 64;                           // Max WiFi password length
static const int maxTimezoneLength = 64;                               // Max timezone string length
static const int minJokeLength = 10;                                   // Minimum joke length to be considered valid

// ============================================================================
// MQTT TOPIC STRUCTURE - DRY Constants
// ============================================================================
namespace MqttTopics {
    // Base namespace
    static const char* NAMESPACE = "scribevolution";

    // Resource types
    static const char* PRINT_RESOURCE = "print";
    static const char* STATUS_RESOURCE = "status";

    // Helper functions
    inline String buildPrintTopic(const String& printerName) {
        return String(NAMESPACE) + "/" + PRINT_RESOURCE + "/" + printerName;
    }

    inline String buildStatusTopic(const String& printerId) {
        return String(NAMESPACE) + "/" + STATUS_RESOURCE + "/" + printerId;
    }

    inline String buildStatusSubscription() {
        return String(NAMESPACE) + "/" + STATUS_RESOURCE + "/+";
    }

    inline String getStatusPrefix() {
        return String(NAMESPACE) + "/" + STATUS_RESOURCE + "/";
    }

    inline bool isStatusTopic(const String& topic) {
        return topic.startsWith(getStatusPrefix());
    }
}

// Memo Configuration
static const int MEMO_COUNT = 4;        // Number of configurable memos
static const int MEMO_MAX_LENGTH = 500; // Maximum length per memo

// Default memo content for first boot
static const char *defaultMemo1 =
    "Salutations! Today is [weekday], [date].\n\nIt's [time]. But what is time, really?";
static const char *defaultMemo2 =
    "Magic 8-Ball says → [pick:It is certain|Yes definitely|Outlook good|Signs point to yes|Ask again later|Cannot predict now|Don't count on it|My sources say no|Outlook not so good|Very doubtful]\n\nToday's dare → [pick:Write a love note to your toaster|Sort socks by mood|Teach goldfish algebra|Whisper to lampposts|Polish the moon with a spoon|Expose the pigeon cabal]";
static const char *defaultMemo3 =
    "Next week's lotto numbers: [dice:59], [dice:59], [dice:59], [dice:59], [dice:59], [dice:59]\n\nToss a coin: [coin].\n\nWill you defeat the [pick:Beholder|Mind Flayer|Gelatinous Cube|Displacer Beast|Mimic|Tarrasque|Owlbear|Lich|Dragon|Rust Monster|Hydra|Chimera]? Your roll: [dice:20]";
static const char *defaultMemo4 =
    "Guest WiFi: chumbawumba\nPassword: igetknockeddown\n\nScribe uptime: [uptime]\nScribe Evolution at: [mdns]\nIP: [ip]";

#endif
