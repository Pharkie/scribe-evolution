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

- Pattern: `scribe/{printer_name}/print`
- `printer_name` comes from Settings → Device → Owner name (also shown as the local printer name in the UI).
- Examples:
  - `scribe/alice/print`
  - `scribe/office-main/print`
  - `scribe/kitchen-home/print`

## Message Formats

Scribe expects a structured JSON payload over MQTT with at least `header` and `body`. The device prints `header`, a blank line, then `body`.

Required fields
- `header` string: e.g. `"MESSAGE"`, `"JOKE"`, `"RIDDLE"`, `"QUOTE"`, `"QUIZ"`, or `"MEMO 1"`–`"MEMO 4"`.
- `body` string: the content to print.

Optional fields
- `sender` string: appended to header as “<header> from <sender>”.
- `timestamp` string: ignored on receipt; device adds its own on print.

Example payloads

```json
{
  "header": "MESSAGE",
  "body": "Remember to pick up milk, bread, and eggs.",
  "sender": "Alice"
}
```

```json
{
  "header": "JOKE",
  "body": "Why did the scarecrow win an award? Because he was outstanding in his field!"
}
```

Memo placeholders (expanded by the printer when header starts with MEMO):

```json
{
  "header": "MEMO 1",
  "body": "Meet at {{time}} in {{room}}"
}
```

Publishing via CLI (mosquitto_pub)

```bash
mosquitto_pub -h YOUR_HOST -p 8883 \
  -u USER -P PASS \
  --cafile isrg-root-x1.pem \
  -t "scribe/alice/print" \
  -m '{"header":"MESSAGE","body":"Hello from CLI","sender":"Bob"}'
```

Using the device HTTP API to publish via MQTT

POST `/api/print-mqtt`

```json
{
  "topic": "scribe/alice/print",
  "header": "MESSAGE",
  "body": "Hello from REST"
}
```

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
4. Enter cluster endpoint and credentials in the web UI (Settings → MQTT)

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
  --cafile isrg-root-x1.pem \
  -t "scribe/alice/print" \
  -m '{"header":"MESSAGE","body":"Test message from command line"}'
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

**Home Assistant** (example action):

```yaml
service: mqtt.publish
data:
  topic: scribe/kitchen/print
  payload: |
    {"header":"MESSAGE","body":"Good morning! Coffee is ready."}
```

**Node-RED**:
Create flows that trigger printing based on:

- Weather alerts
- Calendar events
- Sensor readings
- Time-based schedules

## Security

- TLS: Connections use TLS with CA verification (ISRG Root X1). Configure broker host/port/credentials in Settings → MQTT.
- Trust model: Payloads aren’t signed; printers trust messages that reach their subscribed topic. Protect topics via broker auth/ACLs.
- Sensitive data: Device avoids logging secrets; keep broker creds private; prefer per‑device users.

## External Services

See the dedicated guides for specific integrations:

- [Apple Shortcuts Integration](apple-shortcuts.md)
- [Pipedream Integration](pipedream-integration.md) (Recommended HTTP-to-MQTT bridge)

These bridges convert HTTP webhooks to MQTT messages, enabling integration with services that don't natively support MQTT.
