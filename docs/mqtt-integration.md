# MQTT Integration Guide

This document covers MQTT configuration, remote printing setup, and message formats for networked communication between multiple Scribe Evolution devices.

## Overview

The Scribe Evolution MQTT integration enables:
- **Remote printing** between multiple Scribe Evolution devices
- **Unified web interface** for local and remote printer control
- **Secure TLS communication** via cloud MQTT brokers
- **Quick action buttons** that work with any configured printer
- **External service integration** via webhook-to-MQTT bridges

## Features

### Multi-Printer Support
- Configure multiple remote Scribe Evolution printers in `config.h`
- Single web interface controls all configured printers
- Printer selection in web UI
- Automatic printer discovery and status monitoring

### Secure Communication
- TLS encryption (port 8883) for secure MQTT communication
- Anonymous TLS mode (no certificate verification required)
- Compatible with cloud MQTT brokers like HiveMQ Cloud
- 4096-byte message buffer for large content

### Content Distribution
- All quick action buttons work with remote printers
- Unified endpoint system processes both local and MQTT requests
- Watchdog timer integration prevents system crashes during content generation

## Configuration

### MQTT Broker Setup

Add your MQTT broker credentials to `src/config.h`:

```cpp
// MQTT Configuration
static const char *mqttServer = "your-broker.hivemq.cloud";
static const int mqttPort = 8883;  // TLS port
static const char *mqttUsername = "your-username";
static const char *mqttPassword = "your-password";
```

### Printer Configuration

Configure your local printer and remote printers:

```cpp
// This device's MQTT configuration
static const char *localPrinter[2] = {"Your Printer", "scribeprinter/yourname/inbox"};

// Remote printer configurations
static const char *otherPrinters[][2] = {
    {"Friend's Printer", "scribeprinter/friend/inbox"},
    {"Office Printer", "scribeprinter/office/inbox"},
    {"Kitchen Printer", "scribeprinter/kitchen/inbox"}
};

// Number of other printers configured
static const int numOtherPrinters = 3;
```

### Topic Naming Convention

Use a consistent topic structure:
- **Pattern**: `scribeprinter/{unique-id}/inbox`
- **Examples**:
  - `scribeprinter/alice/inbox`
  - `scribeprinter/office-main/inbox` 
  - `scribeprinter/kitchen-home/inbox`

This ensures topics are unique and organized for easy management.

## Message Formats

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

### Message Processing

- Messages are processed through the unified endpoint system
- Timestamps are automatically added to all printed content
- Watchdog timer prevents system crashes during content generation
- Invalid JSON falls back to plain text printing

## Web Interface Integration

### Printer Selection

The web interface includes a printer selection dropdown:
- **Local Direct** - Print directly to local printer (bypasses MQTT)
- **Local via MQTT** - Print to local printer via MQTT (useful for testing)
- **Remote Printers** - All configured remote printers from `config.h`

### Quick Action Buttons

All quick actions work with any selected printer:
- **🧩 Riddle** - Random riddle from built-in collection (545+ riddles)
- **😂 Joke** - Dad joke from icanhazdadjoke.com API
- **💭 Quote** - Inspirational quote from ZenQuotes.io API  
- **🧠 Quiz** - Trivia question from The Trivia API
- **🔤 Test Print** - Comprehensive character set test for calibration

### User Experience

- Actions provide immediate feedback via toast messages
- Failed remote printing attempts show appropriate error messages
- Printer status indicators show connection state
- Seamless switching between local and remote printers

## MQTT Broker Options

### HiveMQ Cloud (Recommended)

**Free Tier Features**:
- 100 MQTT connections
- 1GB data transfer/month
- TLS encryption included
- Web dashboard for monitoring

**Setup Steps**:
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
  -t "scribeprinter/test/inbox" \
  -m '{"message": "Test message from command line"}'
```

**GUI Clients**:
- **MQTT Explorer** - Visual topic browser
- **MQTTX** - Cross-platform client
- **HiveMQ Web Client** - Browser-based testing

### Diagnostic Information

The Scribe Evolution web interface provides MQTT diagnostic information:
- Connection status
- Subscribed topics
- Last message timestamp
- Error messages and reconnection attempts

## Security Considerations

### Network Security
- Always use TLS encryption (port 8883)
- Avoid plain MQTT (port 1883) for production
- Use strong, unique passwords for MQTT credentials
- Consider firewall rules for self-hosted brokers

### Topic Security
- Use unique, non-guessable topic names
- Consider topic-based access control if supported by broker
- Monitor for unauthorized access in broker logs
- Rotate credentials periodically

### Content Security
- Validate message content before printing
- Consider message size limits to prevent abuse
- Monitor for spam or inappropriate content
- Implement rate limiting if needed

## Troubleshooting

### Connection Issues

**Can't connect to MQTT broker**:
- Check network connectivity
- Verify broker hostname and port
- Confirm credentials are correct
- Check TLS certificate issues

**Frequent disconnections**:
- Check network stability
- Verify broker connection limits
- Monitor broker logs for errors
- Check keep-alive settings

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
    command_topic: "scribeprinter/kitchen/inbox"
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