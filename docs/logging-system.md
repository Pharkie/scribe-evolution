# Logging System Guide

The Scribe Evolution project uses a thread-safe serial logging system built on a custom LogManager singleton, providing reliable debug output without concurrent write corruption.

## Overview

The logging system provides detailed information about:

- System operations and boot sequences
- WiFi connectivity and network status
- MQTT message handling and broker connections
- Printer activity and communication
- API responses and content generation
- Error conditions and debugging information

## Architecture

### LogManager Singleton

The logging system is built around a thread-safe singleton pattern:

- **FreeRTOS Queue**: All log messages are enqueued through a FreeRTOS queue
- **Single Writer Task**: Dedicated writer task prevents concurrent Serial corruption
- **Non-blocking**: Log calls never block (messages dropped if queue full)
- **ISR-safe**: `logfISR()` method for interrupt contexts
- **ESP-IDF Integration**: Captures ESP_LOGx macros via `esp_log_set_vprintf()`

### Thread Safety

The LogManager solves a critical problem in multi-threaded ESP32 applications:

**Problem**: Multiple tasks (AsyncWebServer, MQTT callbacks, button handlers, main loop) calling `Serial.print()` simultaneously causes garbled output and memory corruption.

**Solution**: All logging funnels through a FreeRTOS queue to a single writer task that serializes all Serial writes.

```
Task 1 (Web Handler)  →  \
Task 2 (MQTT Callback) →   > FreeRTOS Queue → Writer Task → Serial.print()
Task 3 (Button Handler) → /
Main Loop              → /
```

### Features

- **Timestamps**: Automatic timestamp prefixes using ezTime library
- **Component Tags**: Messages tagged with source component (WiFi, MQTT, Printer, etc.)
- **Formatted Output**: Clean, readable format with consistent structure
- **Log Levels**: Configurable verbosity (VERBOSE, NOTICE, WARNING, ERROR)

## Configuration

Configure logging in `src/config/device_config.h` (copy from `.example` first):

```cpp
// Application logging levels (custom LOG_* macros)
// Options: LOG_LEVEL_VERBOSE, LOG_LEVEL_NOTICE, LOG_LEVEL_WARNING, LOG_LEVEL_ERROR
static const int logLevel = LOG_LEVEL_NOTICE;

// ESP-IDF system logging levels (ESP_LOGx macros)
// Options: ESP_LOG_VERBOSE, ESP_LOG_INFO, ESP_LOG_WARN, ESP_LOG_ERROR, ESP_LOG_NONE
static const esp_log_level_t espLogLevel = ESP_LOG_WARN;
```

### Log Levels

| Level               | Value | Description              | Includes                                       |
| ------------------- | ----- | ------------------------ | ---------------------------------------------- |
| `LOG_LEVEL_ERROR`   | 2     | Errors only              | Connection failures, API errors                |
| `LOG_LEVEL_WARNING` | 3     | Warnings and errors      | Configuration issues, retry attempts           |
| `LOG_LEVEL_NOTICE`  | 4     | Notice, warnings, errors | Important state changes, successful operations |
| `LOG_LEVEL_VERBOSE` | 6     | All output               | Debug details, variable values                 |

### Development vs Production Settings

**Development**:

```cpp
static const int logLevel = LOG_LEVEL_VERBOSE;
static const esp_log_level_t espLogLevel = ESP_LOG_INFO;
```

**Production**:

```cpp
static const int logLevel = LOG_LEVEL_NOTICE;
static const esp_log_level_t espLogLevel = ESP_LOG_WARN;
```

## Usage

### Basic Logging

```cpp
#include "core/logging.h"

// Log at different severity levels
LOG_ERROR("WIFI", "Failed to connect after %d attempts", retryCount);
LOG_WARNING("MQTT", "Connection unstable, reconnecting...");
LOG_NOTICE("PRINTER", "Print job completed successfully");
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
LOG_VERBOSE("API", "Processing request: %s", endpoint);
LOG_VERBOSE("CONTENT", "Generated %d character message", messageLength);
```

## Serial Monitor Access

**Purpose**: Development debugging and real-time monitoring

**Format**:

```
[2025-01-15 14:30:15] [NOTICE] I: [scribe-kitchen] [WiFi] Connected to MyNetwork
[2025-01-15 14:30:16] [ERROR] E: [scribe-kitchen] [MQTT] Connection failed: timeout
```

**Commands**:

```bash
# PlatformIO
pio device monitor

# Arduino IDE
Tools → Serial Monitor (115200 baud)

# Save logs to file
pio device monitor > scribe.log
```

## Log Analysis and Debugging

### Common Log Patterns

**Boot Sequence**:

```
[2025-01-15 14:30:10] [NOTICE] I: [scribe] [SYSTEM] Boot started, firmware v0.2.0
[2025-01-15 14:30:11] [NOTICE] I: [scribe] [WIFI] Connecting to network...
[2025-01-15 14:30:13] [NOTICE] I: [scribe] [WIFI] Connected, IP: 192.168.1.100
[2025-01-15 14:30:14] [NOTICE] I: [scribe] [MQTT] Connecting to broker...
[2025-01-15 14:30:15] [NOTICE] I: [scribe] [MQTT] Connected and subscribed
[2025-01-15 14:30:15] [NOTICE] I: [scribe] [PRINTER] Initialization complete
[2025-01-15 14:30:16] [NOTICE] I: [scribe] [SYSTEM] Boot completed in 6.2s
```

**Error Conditions**:

```
[ERROR] E: [scribe] [WIFI] Connection timeout after 30s
[WARNING] W: [scribe] [WIFI] Retrying connection (attempt 2/3)
[ERROR] E: [scribe] [MQTT] Broker unreachable: network error
```

**Normal Operations**:

```
[NOTICE] I: [scribe] [API] Processing joke request from 192.168.1.50
[VERBOSE] V: [scribe] [CONTENT] Fetching from icanhazdadjoke.com
[VERBOSE] V: [scribe] [HTTP] Response received: 200 OK, 156 bytes
[NOTICE] I: [scribe] [PRINTER] Message queued for printing
[VERBOSE] V: [scribe] [PRINTER] Print job completed in 2.3s
```

### Debugging Workflow

1. **Set appropriate log level**:
   - `VERBOSE` for detailed debugging
   - `NOTICE` for normal operation monitoring
   - `ERROR` for production error tracking

2. **Monitor serial output**:
   - Real-time development debugging
   - Capture to file for analysis

3. **Analyze log patterns**:
   - Boot sequence completeness
   - Error frequency and patterns
   - Performance timing information
   - Network connectivity issues

4. **Filter by component**:
   - Use `grep` to focus on specific subsystems
   - Track error propagation
   - Identify performance bottlenecks

```bash
# Example: Filter for MQTT-related logs
pio device monitor | grep "\[MQTT\]"

# Example: Show only errors and warnings
pio device monitor | grep -E "\[(ERROR|WARNING)\]"
```

## Performance Considerations

### Log Level Impact

- **VERBOSE**: Significant performance impact, development only
- **NOTICE**: Minimal impact, recommended for production
- **WARNING**: Very low impact, always safe
- **ERROR**: Negligible impact, always safe to enable

### Memory Usage

- **Log queue**: 256 message pointers (default, configurable)
- **Message buffer**: 512 bytes max per message
- **Heap allocation**: Each message allocates heap, freed after writing
- **Queue overflow**: Messages dropped silently if queue full

### Thread Safety Guarantees

✅ **Safe**: Multiple tasks calling LOG\_\* macros simultaneously
✅ **Safe**: ISR contexts using logfISR()
✅ **Safe**: ESP_LOGx macros from ESP-IDF components
❌ **Unsafe**: Direct Serial.print() calls (bypasses LogManager)

## Advanced: Direct LogManager Usage

For cases where you need direct access to the LogManager:

```cpp
#include "core/LogManager.h"

// Regular logging
LogManager::instance().logf("Temperature: %.2f°C\n", temp);

// ISR-safe logging
void IRAM_ATTR buttonISR() {
    LogManager::instance().logfISR("Button interrupt triggered\n");
}
```

## Remote Logging (Optional)

While Scribe Evolution uses serial-only logging for simplicity and thread safety, you can add remote logging via external tools:

### Option 1: Serial-to-MQTT Bridge

```bash
# Pipe serial output to MQTT
pio device monitor | mosquitto_pub -l -t scribe/logs
```

### Option 2: BetterStack Log Shipper

Configure BetterStack's agent to read from serial port or log file.

### Option 3: Syslog Forwarder

Use a host-side script to forward serial output to syslog server.

## Troubleshooting

### No log output

- Check log level is not too restrictive
- Verify serial monitor baud rate (115200)
- Ensure USB cable supports data (not power-only)

### Garbled output

- ⚠️ **Critical**: Never call `Serial.print()` directly - always use LOG\_\* macros
- Check for direct Serial writes bypassing LogManager
- Verify LogManager is initialized in setup()

### Missing timestamps

- Check ezTime library is initialized
- Verify NTP sync completed successfully
- Ensure WiFi connected before logging

### Queue overflow (messages dropped)

- Reduce log level to decrease message volume
- Increase queue size in LogManager::begin() call
- Check for logging inside tight loops

## Best Practices

### Production Deployment

1. **Set NOTICE log level** for balance of information and performance
2. **Use WARNING for ESP-IDF** to reduce system noise
3. **Monitor serial during deployment** to catch issues
4. **Save logs to file** for post-incident analysis

### Development Workflow

1. **Use VERBOSE logging** during active development
2. **Enable ESP-IDF INFO** for detailed system info
3. **Test with different log levels** to verify message importance
4. **Clean up debug messages** before production deployment

### Message Quality

1. **Use descriptive component tags** for easy filtering
2. **Include relevant context** (error codes, values, states)
3. **Keep messages concise** but informative
4. **Use consistent formatting** across similar operations
5. **Avoid logging sensitive information** (passwords, tokens)
6. **Never log in tight loops** (causes queue overflow)

### Thread Safety

1. **Always use LOG\_\* macros** - never call Serial.print() directly
2. **Use logfISR()** in interrupt handlers
3. **Avoid logging from ISRs** unless absolutely necessary
4. **Keep log messages short** in ISR contexts

## Technical Details

### LogManager Implementation

- **Queue size**: 256 message pointers (configurable)
- **Max line length**: 512 bytes (configurable)
- **Writer task stack**: 4096 bytes
- **Writer task priority**: configMAX_PRIORITIES - 2
- **Writer task core**: Core 0 (isolated from AsyncWebServer on Core 1)

### Initialization

LogManager is initialized in `main.cpp`:

```cpp
// Initialize LogManager - provides thread-safe single-writer logging
LogManager::instance().begin(115200, 256, 512);
// Parameters: baud rate, queue length, max line length
```

### Integration with ESP-IDF

LogManager registers as the ESP-IDF vprintf handler, capturing all ESP_LOGx output:

```cpp
esp_log_set_vprintf(espLogVprintf);
```

This ensures thread-safe output for both application (LOG\_\*) and system (ESP_LOGx) logging.
