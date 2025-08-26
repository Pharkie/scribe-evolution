#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

// Increase MQTT packet size before including PubSubClient
#ifdef MQTT_MAX_PACKET_SIZE
#undef MQTT_MAX_PACKET_SIZE
#endif
#define MQTT_MAX_PACKET_SIZE 4096

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"

// External MQTT objects
extern WiFiClientSecure wifiSecureClient;
extern PubSubClient mqttClient;
extern unsigned long lastMQTTReconnectAttempt;
extern const unsigned long mqttReconnectInterval;

// Function declarations
void setupMQTT();
void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void handleMQTTMessage(String topic, String message);
void handleMQTTConnection();
void updateMQTTSubscription();
void setupMQTTWithDiscovery();

#endif // MQTT_HANDLER_H
