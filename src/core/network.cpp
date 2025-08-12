#include "network.h"
#include "logging.h"
#include "config_utils.h"
#include "config_loader.h"
#include <esp_task_wdt.h>

// Network status variables
unsigned long lastReconnectAttempt = 0;
WiFiConnectionMode currentWiFiMode = WIFI_MODE_DISCONNECTED;
DNSServer dnsServer;

// LED status variables
unsigned long lastLEDBlink = 0;
bool ledState = false;

// === LED Status Management ===
void initializeStatusLED()
{
    pinMode(statusLEDPin, OUTPUT);
    digitalWrite(statusLEDPin, LOW);
    // Status LED initialized
}

void updateStatusLED()
{
    unsigned long now = millis();

    switch (currentWiFiMode)
    {
    case WIFI_MODE_CONNECTING:
        // Fast blink (250ms on/off) when trying to connect
        if (now - lastLEDBlink > 250)
        {
            ledState = !ledState;
            digitalWrite(statusLEDPin, ledState);
            lastLEDBlink = now;
        }
        break;

    case WIFI_MODE_STA_CONNECTED:
        // Solid on when connected to WiFi
        digitalWrite(statusLEDPin, HIGH);
        break;

    case WIFI_MODE_AP_FALLBACK:
        // Slow blink (1000ms on/off) when in AP mode
        if (now - lastLEDBlink > 1000)
        {
            ledState = !ledState;
            digitalWrite(statusLEDPin, ledState);
            lastLEDBlink = now;
        }
        break;

    case WIFI_MODE_DISCONNECTED:
    default:
        // LED off when disconnected
        digitalWrite(statusLEDPin, LOW);
        break;
    }
}

// === Configuration Validation ===
void validateConfig()
{
    if (!loadRuntimeConfig())
    {
        // First-time startup: Loading default configuration from config.h
    }

    ValidationResult result = validateDeviceConfig();

    if (!result.isValid)
    {
        Serial.println("❌ Configuration validation FAILED:");
        Serial.print("  ERROR: ");
        Serial.println(result.errorMessage);
        // Critical configuration error - must be visible
    }
} // === WiFi Connection with Fallback AP Mode ===
WiFiConnectionMode connectToWiFi()
{
    const RuntimeConfig &config = getRuntimeConfig();

    // Get WiFi credentials from runtime config, not hardcoded defaults
    String ssid = config.wifiSSID;
    String password = config.wifiPassword;
    unsigned long timeout = config.wifiConnectTimeoutMs;

    if (ssid.length() == 0)
    {
        // No WiFi SSID configured, starting fallback AP mode
        startFallbackAP();
        return WIFI_MODE_AP_FALLBACK;
    }

    currentWiFiMode = WIFI_MODE_CONNECTING;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();

    // Wait for connection with timeout
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout)
    {
        updateStatusLED(); // Fast blink during connection
        delay(500);
        // Connection attempt in progress

        // Feed watchdog during connection attempt
        esp_task_wdt_reset();
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        currentWiFiMode = WIFI_MODE_STA_CONNECTED;
        Serial.print("WiFi connected: ");
        Serial.println(WiFi.localIP());
        return WIFI_MODE_STA_CONNECTED;
    }
    else
    {
        // WiFi connection failed, starting AP mode
        startFallbackAP();
        return WIFI_MODE_AP_FALLBACK;
    }
}

// === Fallback AP Mode Setup ===
void startFallbackAP()
{
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(fallbackAPSSID, fallbackAPPassword);

    if (apStarted)
    {
        currentWiFiMode = WIFI_MODE_AP_FALLBACK;
        IPAddress apIP = WiFi.softAPIP();
        Serial.print("AP Mode: ");
        Serial.print(fallbackAPSSID);
        Serial.print(" -> http://");
        Serial.print(apIP);
        Serial.println("/settings.html");

        // Start DNS server for captive portal (redirect all requests to settings)
        dnsServer.start(53, "*", apIP);
        // DNS server started for captive portal
    }
    else
    {
        currentWiFiMode = WIFI_MODE_DISCONNECTED;
        Serial.println("Failed to start AP mode!");
    }
}

// === mDNS Setup ===
void setupmDNS()
{
    if (MDNS.begin(getMdnsHostname()))
    {
        Serial.println("mDNS responder started");
        Serial.println("Access the form at: http://" + String(getMdnsHostname()) + ".local");

        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", webServerPort);
    }
    else
    {
        Serial.println("Error setting up mDNS responder!");
    }

    // Feed watchdog after potentially slow mDNS operations
    esp_task_wdt_reset();

    LOG_VERBOSE("NETWORK", "mDNS set up");
}

// === WiFi Reconnection Handler ===
void handleWiFiReconnection()
{
    // Only attempt reconnection if we're supposed to be in STA mode
    // but not currently connected, and we're not in AP fallback mode
    if (currentWiFiMode != WIFI_MODE_AP_FALLBACK && WiFi.status() != WL_CONNECTED)
    {
        if (millis() - lastReconnectAttempt > reconnectInterval)
        {
            const RuntimeConfig &config = getRuntimeConfig();

            // Only try to reconnect if we have WiFi credentials
            if (config.wifiSSID.length() > 0)
            {
                LOG_VERBOSE("NETWORK", "WiFi disconnected, attempting reconnection...");
                currentWiFiMode = WIFI_MODE_CONNECTING;

                // Feed watchdog before potentially blocking WiFi operation
                esp_task_wdt_reset();

                WiFi.begin(config.wifiSSID.c_str(), config.wifiPassword.c_str());
                lastReconnectAttempt = millis();

                // Give it a moment to start connecting
                delay(1000);

                // Check if connection was successful
                if (WiFi.status() == WL_CONNECTED)
                {
                    currentWiFiMode = WIFI_MODE_STA_CONNECTED;
                    LOG_VERBOSE("NETWORK", "WiFi reconnected successfully");
                }
                else
                {
                    currentWiFiMode = WIFI_MODE_DISCONNECTED;
                }
            }
        }
    }

    // Always update LED status
    updateStatusLED();
}

// === Utility Functions ===
bool isAPMode()
{
    return currentWiFiMode == WIFI_MODE_AP_FALLBACK;
}

void handleDNSServer()
{
    if (currentWiFiMode == WIFI_MODE_AP_FALLBACK)
    {
        dnsServer.processNextRequest();
    }
}
