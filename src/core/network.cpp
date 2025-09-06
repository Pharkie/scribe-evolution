#include "network.h"
#include "logging.h"
#include "config_utils.h"
#include "config_loader.h"
#include <content/content_generators.h>
#include <utils/content_actions.h>
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
// === WiFi Connection with Fallback AP Mode ===
WiFiConnectionMode connectToWiFi()
{
    const RuntimeConfig &config = getRuntimeConfig();

    // Get WiFi credentials from runtime config, not hardcoded defaults
    String ssid = config.wifiSSID;
    String password = config.wifiPassword;
    unsigned long timeout = config.wifiConnectTimeoutMs;

    Serial.printf("[BOOT] Network: Connecting to WiFi (timeout: %lus)\n", timeout/1000);

    if (ssid.length() == 0)
    {
        // No WiFi SSID configured, starting fallback AP mode
        Serial.println("[BOOT] Network: No SSID configured - starting AP mode");
        startFallbackAP();
        return WIFI_MODE_AP_FALLBACK;
    }

    currentWiFiMode = WIFI_MODE_CONNECTING;

    WiFi.mode(WIFI_STA);

    // Quick scan to avoid long blind wait if SSID isn't around
    LOG_NOTICE("NETWORK", "Scanning for target SSID before connecting...");
    // Blocking scan; typically a few seconds total, much less than a 15s timeout
    int16_t foundCount = WiFi.scanNetworks();
    bool ssidPresent = false;
    for (int i = 0; i < foundCount; i++)
    {
        if (WiFi.SSID(i) == ssid)
        {
            ssidPresent = true;
            break;
        }
    }
    WiFi.scanDelete();

    if (!ssidPresent)
    {
        Serial.println("[BOOT] Network: Target SSID not found - starting AP mode");
        startFallbackAP();
        return WIFI_MODE_AP_FALLBACK;
    }

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
        long rssi = WiFi.RSSI();
        Serial.printf("[BOOT] Network: ✅ Connected to WiFi (RSSI: %ld dBm)\n", rssi);
        Serial.printf("[BOOT] Network: IP address: %s\n", WiFi.localIP().toString().c_str());
        return WIFI_MODE_STA_CONNECTED;
    }
    else
    {
        // WiFi connection failed, starting AP-STA fallback
        startFallbackAP();
        return WIFI_MODE_AP_FALLBACK;
    }
}

// === Fallback AP Mode Setup ===
void startFallbackAP()
{
    WiFi.mode(WIFI_AP_STA);
    bool apStarted = WiFi.softAP(fallbackAPSSID, fallbackAPPassword);

    if (apStarted)
    {
        currentWiFiMode = WIFI_MODE_AP_FALLBACK;
        IPAddress apIP = WiFi.softAPIP();
        Serial.println();
        Serial.println("======================================");
        Serial.println("🔴 DEVICE STARTED IN AP-STA MODE");
        Serial.println("======================================");
        Serial.println("WiFi Network: " + String(fallbackAPSSID));
        Serial.println("WiFi Password: " + String(fallbackAPPassword));
        Serial.println("Setup URL: http://" + apIP.toString() + "/settings/");
        Serial.println("1. Connect to WiFi: " + String(fallbackAPSSID));
        Serial.println("2. Open browser to: " + apIP.toString());
        Serial.println("3. Configure your WiFi settings");
        Serial.println("======================================");
        Serial.println();

        // Start DNS server for captive portal (redirect all requests to settings)
        dnsServer.start(53, "*", apIP);
        // DNS server started for captive portal
    }
    else
    {
        currentWiFiMode = WIFI_MODE_DISCONNECTED;
        Serial.println("Failed to start AP-STA mode!");
    }
}

// === mDNS Setup ===
void setupmDNS()
{
    // Skip mDNS setup in AP mode - it's not useful and creates confusion
    if (isAPMode())
    {
        Serial.println("Skipping mDNS setup (AP-STA mode - use IP address instead)");
        return;
    }

    if (MDNS.begin(getMdnsHostname()))
    {
        Serial.printf("[BOOT] mDNS: http://%s.local\n", getMdnsHostname());

        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", webServerPort);
    }
    else
    {
        Serial.println("[BOOT] mDNS: ❌ Setup failed");
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

bool isAPSTAMode()
{
    // Currently AP-STA mode is represented by WIFI_MODE_AP_FALLBACK
    return isAPMode();
}

void handleDNSServer()
{
    if (currentWiFiMode == WIFI_MODE_AP_FALLBACK)
    {
        // Process DNS requests - this will redirect all domain queries to our AP IP
        dnsServer.processNextRequest();
    }
}

// === AP Details Printing ===
void printAPDetailsOnStartup()
{
    // Only print if we're actually in AP mode
    if (currentWiFiMode != WIFI_MODE_AP_FALLBACK)
    {
        return;
    }
    
    // Generate the AP details content using the shared function
    String apContent = generateAPDetailsContent();
    
    if (apContent.length() > 0)
    {
        // Create a content action result and queue for printing
        ContentActionResult result(true, "NETWORK INFO", apContent);
        if (queueContentForPrinting(result))
        {
            LOG_NOTICE("NETWORK", "AP connection details queued for printing");
        }
        else
        {
            LOG_ERROR("NETWORK", "Failed to queue AP details for printing");
        }
    }
    else
    {
        LOG_ERROR("NETWORK", "Failed to generate AP details content");
    }
}
