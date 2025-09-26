# MQTT Integration Guide

This document covers MQTT configuration, remote printing setup, and message formats for networked communication between multiple Scribe Evolution devices.

## Overview

The Scribe Evolution MQTT integration enables:

- **Remote printing** between multiple Scribe Evolution devices
- **Unified web interface** for local and remote printer control
- **Secure TLS communication** via cloud MQTT brokers
- **Quick action buttons** that work with any configured printer
- **External service integration** via webhook-to-MQTT bridges

## Configuration

### MQTT Broker Setup

Set credentials via Settings → MQTT (persisted in NVS).


### Topic Naming Convention

- **Pattern**: `scribeprinter/{unique-id}/print`
- **Examples**:
  - `scribeprinter/alice/print`
  - `scribeprinter/office-main/print`
  - `scribeprinter/kitchen-home/print`

## Message Formats

Note: the below may need updating.. check what the web interface sends to be sure.

### Simple Text Messages

For direct text printing:

```json
{
  "message": "Your text content to print"
}
```

**Example**:

```json
{
  "message": "Remember to pick up groceries: milk, bread, eggs"
}
```

### Endpoint Actions

For triggering quick actions remotely:

```json
{"endpoint": "/joke"}
{"endpoint": "/riddle"}
{"endpoint": "/quote"}
{"endpoint": "/quiz"}
{"endpoint": "/test"}
```

These endpoints trigger the same functionality as the web interface quick action buttons.

## MQTT Broker Options

### HiveMQ Cloud (Recommended)

**Free Tier Features**:

- 100 MQTT connections
- 1GB data transfer/month
- TLS encryption included
- Web dashboard for monitoring

**Setup**:

1. Create account at [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/)
2. Create cluster and note connection details
3. Create credentials for your devices
4. Update `config.h` with cluster endpoint and credentials

### Other Cloud Brokers

**AWS IoT Core**:

- Part of AWS ecosystem
- Certificate-based authentication
- Pay-per-use pricing

**Azure IoT Hub**:

- Microsoft's IoT platform
- Device management features
- Free tier available

**Self-Hosted Options**:

- **Mosquitto** - Lightweight, popular choice
- **EMQX** - High-performance, web dashboard
- **VerneMQ** - Distributed, scalable

## Testing MQTT Connection

### Using MQTT Client Tools

Test your setup with MQTT client tools:

**Command line (mosquitto_pub)**:

```bash
mosquitto_pub -h your-broker.hivemq.cloud -p 8883 \
  -u your-username -P your-password \
  -t "scribeprinter/test/print" \
  -m '{"message": "Test message from command line"}'
```

**GUI Clients**:

- **MQTT Explorer** - Visual topic browser
- **MQTTX** - Cross-platform client
- **HiveMQ Web Client** - Browser-based testing

### Diagnostic Information

Diagnostics page shows connection status, subscriptions, last message, and errors.

### Message Issues

**Messages not printing**:

- Verify topic name matches configuration
- Check JSON format validity
- Monitor for MQTT delivery confirmation
- Check printer hardware and paper

**Delayed messages**:

- Check broker message queue
- Verify network latency
- Monitor broker performance
- Consider local broker for latency-sensitive applications

### Debugging Commands

Enable verbose logging in `config.h`:

```cpp
static const int logLevel = LOG_LEVEL_VERBOSE;
```

Monitor serial output for detailed MQTT connection and message processing logs.

## Integration Examples

### Home Automation

**Home Assistant**:

```yaml
mqtt:
  - name: "Scribe Evolution Kitchen Printer"
    command_topic: "scribe/kitchen/print"
    payload_on: '{"message": "Good morning! Coffee is ready."}'
```

**Node-RED**:
Create flows that trigger printing based on:

- Weather alerts
- Calendar events
- Sensor readings
- Time-based schedules

### External Services

See the dedicated guides for specific integrations:

- [Apple Shortcuts Integration](apple-shortcuts.md)
- [Pipedream Integration](pipedream-integration.md) (Recommended HTTP-to-MQTT bridge)

These bridges convert HTTP webhooks to MQTT messages, enabling integration with services that don't natively support MQTT.
