# Logging System Guide

The Scribe project includes a comprehensive logging system built on the ArduinoLog library, providing multiple output destinations and configurable log levels for debugging and monitoring.

## Overview

The logging system provides detailed information about:

- System operations and boot sequences
- WiFi connectivity and network status
- MQTT message handling and broker connections
- Printer activity and communication
- API responses and content generation
- Error conditions and debugging information

## Features

### Multiple Output Destinations

1. **Serial Console** - Development debugging via USB serial connection
2. **LittleFS File Storage** - Persistent logs saved to `/logs/scribe.log`
3. **MQTT Topic Publishing** - Remote monitoring via `scribe/log` topic
4. **BetterStack Integration** - Cloud log aggregation service

### Configurable Log Levels

The system uses ArduinoLog's standard severity levels:

| Level               | Value | Description                             | Includes                                       |
| ------------------- | ----- | --------------------------------------- | ---------------------------------------------- |
| `LOG_LEVEL_SILENT`  | 0     | No output                               | Nothing                                        |
| `LOG_LEVEL_FATAL`   | 1     | Fatal errors only                       | System crashes, critical failures              |
| `LOG_LEVEL_ERROR`   | 2     | Errors and fatals                       | Connection failures, API errors                |
| `LOG_LEVEL_WARNING` | 3     | Warnings, errors, fatals                | Configuration issues, retry attempts           |
| `LOG_LEVEL_NOTICE`  | 4     | Notice, warnings, errors, fatals        | Important state changes, successful operations |
| `LOG_LEVEL_TRACE`   | 5     | Trace, notice, warnings, errors, fatals | Function entry/exit, detailed flow             |
| `LOG_LEVEL_VERBOSE` | 6     | All output                              | Debug details, variable values                 |

### Structured Logging

- **Timestamps** - Automatic timestamp prefixes using ezTime library
- **Component Tags** - Messages tagged with source component (WiFi, MQTT, Printer, etc.)
- **Formatted Output** - Clean, readable format with consistent structure
- **Multi-Output Handling** - Single log call outputs to all enabled destinations

## Configuration

Configure logging in `src/core/config.h` (copy from `.example` first):

```cpp
// Set your desired log level (0-6)
static const int logLevel = LOG_LEVEL_NOTICE;

// Enable/disable output destinations
static const bool enableSerialLogging = true;       // Serial console
static const bool enableFileLogging = false;        // LittleFS file (/logs/scribe.log)
static const bool enableMQTTLogging = false;        // MQTT topic
static const bool enableBetterStackLogging = false; // BetterStack (HTTP)

// File logging configuration
static const char* logFileName = "/logs/scribe.log";
static const size_t maxLogFileSize = 100000; // 100KB max file size

// BetterStack configuration (if enabled)
static const char *betterStackToken = "YOUR_TOKEN";
static const char *betterStackEndpoint = "https://in.logs.betterstack.com/http/";
```

### Development vs Production Settings

Development example:

```cpp
static const int logLevel = LOG_LEVEL_VERBOSE;
static const bool enableSerialLogging = true;
static const bool enableFileLogging = true;
static const bool enableMQTTLogging = false;
static const bool enableBetterStackLogging = false;
```

Production example:

```cpp
static const int logLevel = LOG_LEVEL_NOTICE;
static const bool enableSerialLogging = false;
static const bool enableFileLogging = true;
static const bool enableMQTTLogging = true;
static const bool enableBetterStackLogging = true;
```

## Usage Examples

### Basic Logging

```cpp
#include "core/logging.h"

// Log at different severity levels
LOG_FATAL("SYSTEM", "Critical system failure: %s", errorMessage);
LOG_ERROR("WIFI", "Failed to connect after %d attempts", retryCount);
LOG_WARNING("MQTT", "Connection unstable, reconnecting...");
LOG_NOTICE("PRINTER", "Print job completed successfully");
LOG_TRACE("API", "Entering function: %s", __func__);
LOG_VERBOSE("DEBUG", "Variable value: temperature=%d", temperature);
```

### Component-Specific Logging

The system uses consistent component tags:

```cpp
// Network components
LOG_NOTICE("WIFI", "Connected to network: %s", ssid);
LOG_ERROR("MQTT", "Broker connection failed: %s", broker);

// Hardware components
LOG_NOTICE("PRINTER", "Character mapping: %c -> %c", input, output);
LOG_WARNING("HARDWARE", "Button press detected on GPIO %d", pin);

// Application components
LOG_TRACE("API", "Processing request: %s", endpoint);
LOG_VERBOSE("CONTENT", "Generated %d character message", messageLength);
```

### Structured Logging Function

For complex logging scenarios:

```cpp
// Use structured logging helper
structuredLog("COMPONENT", LOG_LEVEL_ERROR,
              "Operation failed: %s (code=%d, retries=%d)",
              operation, errorCode, retryCount);
```

## Output Destinations

### Serial Console

**Purpose**: Development debugging and real-time monitoring
**Access**: Connect via USB and use serial monitor at 115200 baud
**Format**:

```
[2025-01-15 14:30:15] [NOTICE] WiFi connected to MyNetwork
[2025-01-15 14:30:16] [ERROR] MQTT connection failed: timeout
```

**Commands**:

```bash
# PlatformIO
pio device monitor

# Arduino IDE
Tools → Serial Monitor (115200 baud)
```

### LittleFS File Storage

**Purpose**: Persistent logging for debugging issues after the fact
**Location**: `/logs/scribe.log` on device filesystem
**Rotation**: Automatic when file exceeds `maxLogFileSize`
**Access**: Download via web interface or direct filesystem access

**File Management**:

- Current log: `/logs/scribe.log`
- Backup log: `/logs/scribe.log.old` (previous rotation)
- Automatic rotation prevents storage exhaustion
- Logs survive device reboots

### MQTT Publishing

**Purpose**: Remote monitoring and centralized log collection
**Topic**: `scribe/log` (configurable)
**Format**: JSON messages with structured data

**Message Structure**:

```json
{
  "timestamp": "2025-01-15T14:30:15Z",
  "level": "ERROR",
  "component": "MQTT",
  "message": "Connection failed: timeout",
  "device": "scribe-kitchen"
}
```

**Monitoring**:

```bash
# Subscribe to log messages
mosquitto_sub -h your-broker.com -p 8883 -u username -P password -t "scribe/log"
```

### BetterStack Integration

**Purpose**: Professional cloud log aggregation and analysis
**Service**: [BetterStack Logs](https://betterstack.com/logs)
**Features**:

- Web dashboard for log analysis
- Alerting and notifications
- Search and filtering
- Long-term retention

**Setup**:

1. Create BetterStack account
2. Create HTTP source and get token
3. Add token to `config.h`
4. Enable `logToBetterStack = true`

## Log Analysis and Debugging

### Common Log Patterns

**Boot Sequence**:

```
[INFO] SYSTEM: Boot started, firmware v1.0.0
[INFO] WIFI: Connecting to network...
[NOTICE] WIFI: Connected, IP: 192.168.1.100
[INFO] MQTT: Connecting to broker...
[NOTICE] MQTT: Connected and subscribed
[INFO] PRINTER: Initialization complete
[NOTICE] SYSTEM: Boot completed in 5.2s
```

**Error Conditions**:

```
[ERROR] WIFI: Connection timeout after 30s
[WARNING] WIFI: Retrying connection (attempt 2/3)
[ERROR] MQTT: Broker unreachable: network error
[FATAL] SYSTEM: Watchdog timer expired, rebooting...
```

**Normal Operations**:

```
[NOTICE] API: Processing joke request from 192.168.1.50
[TRACE] CONTENT: Fetching from icanhazdadjoke.com
[VERBOSE] HTTP: Response received: 200 OK, 156 bytes
[NOTICE] PRINTER: Message queued for printing
[TRACE] PRINTER: Print job completed in 2.3s
```

### Debugging Workflow

1. **Set appropriate log level**:
   - `VERBOSE` for detailed debugging
   - `NOTICE` for normal operation monitoring
   - `ERROR` for production error tracking

2. **Enable relevant outputs**:
   - Serial for real-time development debugging
   - File for persistent issue investigation
   - MQTT for remote monitoring
   - BetterStack for production analysis

3. **Analyze log patterns**:
   - Boot sequence completeness
   - Error frequency and patterns
   - Performance timing information
   - Network connectivity issues

4. **Filter by component**:
   - Focus on specific subsystems
   - Track error propagation
   - Identify performance bottlenecks

## Performance Considerations

### Log Level Impact

- **VERBOSE**: Significant performance impact, development only
- **TRACE**: Moderate impact, detailed debugging
- **NOTICE**: Minimal impact, recommended for production
- **ERROR**: Negligible impact, always safe to enable

### Output Destination Performance

- **Serial**: Fastest, no storage overhead
- **File**: Moderate overhead, impacts flash wear
- **MQTT**: Network dependent, queued for reliability
- **BetterStack**: HTTP overhead, asynchronous

### Memory Usage

- **Log buffer**: 512 bytes for message formatting
- **File rotation**: Prevents unlimited storage growth
- **MQTT queuing**: Limited queue size to prevent memory exhaustion

## Troubleshooting

### Common Issues

**No log output**:

- Check log level is not `LOG_LEVEL_SILENT`
- Verify output destinations are enabled
- Confirm serial baud rate (115200)

**File logging not working**:

- Check LittleFS is properly initialized
- Verify sufficient storage space
- Confirm file path is correct (`/logs/scribe.log`)

**MQTT logging fails**:

- Verify MQTT broker connection is established
- Check MQTT credentials and permissions
- Ensure log topic is allowed for publishing

**BetterStack not receiving logs**:

- Verify token is correct
- Check internet connectivity
- Monitor HTTP request success in debug logs

### Debug Log Configuration

For maximum debugging information:

```cpp
static const int logLevel = LOG_LEVEL_VERBOSE;
static const bool logToSerial = true;
static const bool logToFile = true;
static const bool logToMQTT = true;
static const bool logToBetterStack = false;  // Avoid HTTP overhead during debugging
```

### Log Analysis Tools

**Serial Monitor**: Real-time log streaming
**Text Editors**: Search and filter log files
**MQTT Clients**: Subscribe to remote log streams
**BetterStack Dashboard**: Advanced log analysis and alerting

## Best Practices

### Production Deployment

1. **Set appropriate log level** (`NOTICE` recommended)
2. **Enable file logging** for post-incident analysis
3. **Use MQTT logging** for centralized monitoring
4. **Configure BetterStack** for professional deployments
5. **Monitor storage usage** to prevent filesystem exhaustion

### Development Workflow

1. **Use VERBOSE logging** during active development
2. **Enable serial output** for immediate feedback
3. **Test with different log levels** to verify message importance
4. **Clean up debug messages** before production deployment

### Message Quality

1. **Use descriptive component tags** for easy filtering
2. **Include relevant context** (error codes, values, states)
3. **Keep messages concise** but informative
4. **Use consistent formatting** across similar operations
5. **Avoid logging sensitive information** (passwords, tokens)
