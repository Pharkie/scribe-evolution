#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include "config.h"

// WiFi mode enumeration
enum WiFiConnectionMode
{
    WIFI_MODE_STA_CONNECTED, // Connected to configured WiFi network
    WIFI_MODE_AP_FALLBACK,   // In AP mode due to connection failure
    WIFI_MODE_CONNECTING,    // Attempting to connect to WiFi
    WIFI_MODE_DISCONNECTED   // No WiFi connection
};

// Network status variables
extern unsigned long lastReconnectAttempt;
extern const unsigned long reconnectInterval;
extern WiFiConnectionMode currentWiFiMode;
extern DNSServer dnsServer;

// Function declarations
void validateConfig();
WiFiConnectionMode connectToWiFi();
void startFallbackAP();
void setupmDNS();
void handleWiFiReconnection();
void updateStatusLED();
void initializeStatusLED();
bool isAPMode();
void handleDNSServer();

#endif // NETWORK_H
