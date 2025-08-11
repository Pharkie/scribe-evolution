#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"

// Network status variables
extern unsigned long lastReconnectAttempt;
extern const unsigned long reconnectInterval;

// Function declarations
void validateConfig();
void connectToWiFi();
void setupmDNS();
void handleWiFiReconnection();

#endif // NETWORK_H
