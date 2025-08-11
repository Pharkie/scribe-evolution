#include "network.h"
#include "logging.h"
#include "config_utils.h"
#include <esp_task_wdt.h>

// Network status variables
unsigned long lastReconnectAttempt = 0;

// === Configuration Validation ===
void validateConfig()
{
    Serial.println("=== VALIDATING CONFIGURATION ===");

    // Use the simplified validation
    ValidationResult result = validateDeviceConfig();

    if (result.isValid)
    {
        Serial.println("✓ All configuration validation passed");

        // Display current configuration
        Serial.print("Device owner: ");
        Serial.println(defaultDeviceOwner);
        Serial.print("WiFi SSID: ");
        Serial.println(getWifiSSID());
        Serial.print("Printer name: ");
        Serial.println(getLocalPrinterName());
        Serial.print("MQTT topic: ");
        Serial.println(getLocalPrinterTopic());
        Serial.print("mDNS hostname: ");
        Serial.println(getMdnsHostname());
        Serial.print("Timezone: ");
        Serial.println(getTimezone());

        Serial.println("=== CONFIGURATION VALIDATION COMPLETE - ALL OK ===");
    }
    else
    {
        Serial.println("❌ Configuration validation FAILED:");
        Serial.print("  ERROR: ");
        Serial.println(result.errorMessage);
        Serial.println("=== CONFIGURATION VALIDATION FAILED ===");
        // Note: System will continue but may not function correctly
    }
} // === WiFi Connection ===
void connectToWiFi()
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(getWifiSSID());

    WiFi.begin(getWifiSSID(), getWifiPassword());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("WiFi connected successfully!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println();
        Serial.println("Failed to connect to WiFi - continuing anyway");
        Serial.println("Will attempt reconnection every 30 seconds in main loop");
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
    // Check WiFi connection and reconnect if needed
    if (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - lastReconnectAttempt > reconnectInterval)
        {
            Serial.println("WiFi disconnected, attempting reconnection...");

            // Feed watchdog before potentially blocking WiFi operation
            esp_task_wdt_reset();

            WiFi.begin(getWifiSSID(), getWifiPassword());
            lastReconnectAttempt = millis();
        }
    }
}
