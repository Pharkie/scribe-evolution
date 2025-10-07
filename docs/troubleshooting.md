# Troubleshooting Guide

This document covers common issues, solutions, and diagnostic procedures for the Scribe Evolution ESP32-C3 Thermal Printer project.

## Overview

The Scribe Evolution thermal printer system has been tested extensively and is generally very reliable. Most issues stem from a few common causes that can be systematically diagnosed and resolved.

## Common Issues and Solutions

### Printer Not Working

#### Symptoms

- No output when attempting to print
- Device appears to function but nothing prints
- Printer makes no noise or movement

#### Potential Causes and Solutions

**1. Power Supply Issues**

**Insufficient Current**:

- **Problem**: Power supply can't provide enough current for printing
- **Solution**: Use a 5V power supply capable of 2.4A or higher
- **Test**: Try a different, higher-capacity power supply
- **Symptoms**: Device works for basic functions but fails when printing

**Wrong Power Connection**:

- **Problem**: Printer powered incorrectly or not at all
- **Solution**: Ensure 5V goes to printer POWER connector, not USB
- **Important**: Never power printer through ESP32-C3
- **Test**: Measure 5V between printer POWER VH and GND pins

**Dual Power Source Conflict**:

- **Problem**: ESP32-C3 powered by both USB and 5V pin simultaneously
- **Solution**: Use only ONE power source at a time
- **Development**: Use USB cable only
- **Operation**: Use shared 5V supply for both printer and ESP32-C3

**2. Wiring Issues**

**Incorrect Connections**:

```
Correct Wiring:
ESP32-C3 GPIO20 → Printer TTL RX
ESP32-C3 GND → Printer TTL GND → Power Supply GND
Power Supply 5V → Printer POWER VH
```

**Short Circuits**:

- **Problem**: Wires touching or poor insulation
- **Solution**: Check all connections with multimeter
- **Test**: Measure resistance between power rails (should be high)
- **Prevention**: Use heat shrink tubing on all connections

**Loose Connections**:

- **Problem**: Intermittent contact due to poor soldering
- **Solution**: Re-solder all connections with proper technique
- **Test**: Gently wiggle wires while monitoring functionality

**3. Paper Issues**

**Paper Not Inserted Correctly**:

- **Problem**: Paper not feeding through printer mechanism
- **Solution**: Follow printer manual for correct paper loading
- **Check**: Paper should feed from bottom, printed side should face out

**Wrong Paper Size**:

- **Problem**: Paper too wide, too narrow, or wrong type
- **Solution**: Use thermal paper 57.5±0.5mm width, 30mm max diameter
- **Test**: Try known-good paper roll

**Paper Path Obstruction**:

- **Problem**: Debris or damaged mechanism blocking paper
- **Solution**: Clean printer mechanism, check for obstructions
- **Test**: Manual feed test (hold front button during power-on)

### Network Connectivity Issues

#### WiFi Connection Problems

**Can't Connect to Network**:

- **Check config.h**: Verify SSID and password are correct
- **Signal Strength**: Move closer to router for testing
- **Network Type**: Ensure 2.4GHz network (ESP32-C3 doesn't support 5GHz)
- **Special Characters**: Avoid special characters in WiFi credentials
- **Hidden Networks**: Explicitly enable connection to hidden SSIDs if needed

**Frequent Disconnections**:

- **Power Issues**: Ensure stable power supply
- **Range Issues**: Check signal strength and potential interference
- **Router Issues**: Try connecting other devices to same network
- **Watchdog**: Check logs for watchdog timer resets

#### mDNS Access Issues

**Can't Access scribe.local**:

- **mDNS Support**: Ensure client device supports mDNS (most modern devices do)
- **Network Isolation**: Some networks block mDNS between devices
- **Alternative**: Use IP address directly (shown in boot message)
- **Firewall**: Check client device firewall settings

### MQTT Issues

#### Connection Problems

**Can't Connect to MQTT Broker**:

- **Credentials**: Verify username, password, and server hostname
- **Port**: Ensure correct port (8883 for TLS, 1883 for plain)
- **TLS Configuration**: Check broker supports TLS if using secure port
- **Network**: Test MQTT connection from another client
- **Firewall**: Check if MQTT ports are blocked

**Authentication Failures**:

- **Credentials**: Double-check username and password
- **Permissions**: Ensure MQTT user has publish/subscribe permissions
- **Topic Access**: Verify user can access required topics
- **Connection Limits**: Check if account has reached connection limits

#### Message Issues

**Messages Not Printing**:

- **Topic Names**: Verify topic matches printer configuration exactly
- **Message Format**: Ensure valid JSON format for structured messages
- **QoS Settings**: Try different QoS levels (0, 1, or 2)
- **Retained Messages**: Clear any problematic retained messages

**Delayed Messages**:

- **Broker Performance**: Check broker status and performance
- **Network Latency**: Test network connection speed
- **Queue Backlog**: Check if messages are queuing on broker
- **Processing Time**: Monitor content generation time for complex messages

### Web Interface Issues

#### Can't Access Web Interface

**Page Won't Load**:

- **Network Connection**: Verify device is connected to WiFi
- **URL**: Try both IP address and scribe.local
- **Port**: Ensure accessing correct port (default 80)
- **Browser Cache**: Clear browser cache and cookies
- **Device Status**: Check if device is booted and running

**Partial Loading**:

- **Asset Issues**: CSS or JavaScript files may be corrupted
- **Rebuild Assets**: Run `npm run build` and re-upload
- **Network Issues**: Slow or unstable network connection
- **Browser Compatibility**: Try different browser

#### Functionality Issues

**Buttons Not Working**:

- **JavaScript Errors**: Check browser console for errors
- **Network Issues**: API requests may be failing
- **Event Listeners**: JavaScript event handlers may not be attached
- **Browser Support**: Ensure modern browser with JavaScript enabled

**Settings Not Saving**:

- **NVS Issues**: Non-volatile storage may be corrupted
- **Flash Issues**: Storage may be full or corrupted
- **Validation Errors**: Check for input validation failures
- **Network Issues**: Settings requests may be timing out
- **Auth/CSRF**: If POST fails with 403, ensure the UI was loaded from `/` (session cookie set). The UI sends a CSRF token automatically in headers.

### Content Generation Issues

#### API-Based Content (Jokes, Quotes, Quiz)

**No Content Generated**:

- **Internet Connection**: Verify device has internet access
- **API Status**: Check if external API service is operational
- **Rate Limiting**: May have exceeded API rate limits
- **DNS Issues**: DNS resolution may be failing

**Slow Content Generation**:

- **Network Latency**: High latency to external APIs
- **API Performance**: External service may be slow
- **Timeout Settings**: Increase timeout values if needed
- **Retry Logic**: Check retry mechanisms are working

#### Local Content (Riddles)

**Riddles Not Loading**:

- **File System**: Check LittleFS is properly initialized
- **File Corruption**: Riddles database file may be corrupted
- **Memory Issues**: Insufficient RAM for loading riddle data
- **Flash Issues**: File system may be corrupted

## Diagnostic Procedures

### Serial Monitor Diagnostics

**Enable Verbose Logging**:

```cpp
// In config.h
static const int logLevel = LOG_LEVEL_VERBOSE;
static const bool logToSerial = true;
```

**Key Information to Look For**:

```
Boot Sequence:
[INFO] SYSTEM: Boot started, firmware v1.0.0
[INFO] WIFI: Connecting to network...
[NOTICE] WIFI: Connected, IP: 192.168.1.100

MQTT Connection:
[INFO] MQTT: Connecting to broker...
[NOTICE] MQTT: Connected and subscribed

Printer Communication:
[TRACE] PRINTER: Sending message: "Hello World"
[NOTICE] PRINTER: Print completed successfully
```

**Common Error Patterns**:

```
Power Issues:
[FATAL] SYSTEM: Watchdog timer expired, rebooting...

Network Issues:
[ERROR] WIFI: Connection timeout after 30s
[ERROR] MQTT: Broker unreachable: network error

Printer Issues:
[ERROR] PRINTER: No response from printer
[WARNING] PRINTER: Communication timeout
```

### Multimeter Testing

**Power Rail Testing**:

- Measure 5V between power supply positive and ground
- Measure 3.3V between ESP32-C3 3.3V pin and ground
- Check for voltage drops under load (during printing)

**Continuity Testing**:

- Verify connections between ESP32-C3 and printer
- Check ground continuity throughout system
- Test for short circuits between power rails

**Signal Testing**:

- Monitor GPIO20 (TX) signal with oscilloscope during printing
- Verify signal levels are appropriate (3.3V logic)

### MQTT Client Testing

**Command Line Testing**:

```bash
# Subscribe to printer topic
mosquitto_sub -h your-broker.com -p 8883 -u username -P password -t "scribe/+/inbox"

# Publish test message
mosquitto_pub -h your-broker.com -p 8883 -u username -P password \
  -t "scribe/yourprinter/inbox" \
  -m '{"message":"Test from command line"}'
```

**GUI MQTT Clients**:

- **MQTT Explorer**: Visual topic browser and message inspector
- **MQTTX**: Cross-platform client with advanced features
- **HiveMQ Web Client**: Browser-based testing tool

### Network Diagnostics

**Ping Test**:

```bash
# Test basic connectivity
ping scribe.local
ping 192.168.1.100  # Use actual IP address
```

**Port Testing**:

```bash
# Test web server port
telnet scribe.local 80
nc -zv scribe.local 80
```

**DNS Testing**:

```bash
# Test mDNS resolution
nslookup scribe.local
```

## Hardware-Specific Issues

### CSN-A4L Thermal Printer

**Self-Test Procedure**:

1. Disconnect all cables except power
2. Hold front button while applying 5V power
3. Printer should print test pattern and font samples
4. If no output, printer hardware may be faulty

**Common Printer Issues**:

- **Thermal head failure**: Partial or no printing
- **Motor issues**: Paper feeding problems
- **Controller failure**: No response to commands
- **Power issues**: Insufficient current for operation

**Printer Manual Reference**:
For detailed troubleshooting: [CSN-A4L Manual](https://www.manualslib.com/manual/3035820/Cashino-Csn-A4l.html)

### ESP32-C3 Specific Issues

**Flash Corruption**:

- **Symptoms**: Boot loops, random crashes, strange behavior
- **Solution**: Re-flash firmware completely
- **Prevention**: Use stable power supply, avoid interrupting uploads

**Brown-out Detection**:

- **Symptoms**: Unexpected resets during high current operations
- **Solution**: Improve power supply quality and capacity
- **Monitoring**: Check for voltage drops during printing

## Performance Optimization

### Memory Management

**Monitor Heap Usage**:

```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Minimum free heap: %d bytes\n", ESP.getMinFreeHeap());
```

**Common Memory Issues**:

- Large JSON documents consuming too much RAM
- Memory leaks in long-running operations
- Stack overflow from deep recursion

### Flash Storage Management

**Monitor Flash Usage**:

```cpp
size_t total = ESP.getFlashChipSize();
size_t used = ESP.getSketchSize();
Serial.printf("Flash usage: %d%% (%d/%d bytes)\n", used * 100 / total, used, total);
```

**File System Monitoring**:

- Check available space for logs and assets
- Monitor file system corruption
- Implement log rotation to prevent storage exhaustion

## Getting Help

### Information to Collect

When reporting issues, include:

1. **Hardware Configuration**:
   - ESP32-C3 board model
   - Power supply specifications
   - Printer model and firmware version
   - Wiring diagram or photos

2. **Software Configuration**:
   - Firmware version
   - PlatformIO platform version
   - Library versions
   - Configuration settings (anonymized)

3. **Error Information**:
   - Serial monitor output
   - LED behavior patterns
   - Network status
   - Exact steps to reproduce

4. **Environment Details**:
   - Network configuration
   - MQTT broker details (anonymized)
   - Development platform (OS, IDE version)

### Support Resources

- **GitHub Issues**: Report bugs and request features
- **Documentation**: Check all documentation files for detailed information
- **Community**: Share experiences and solutions
- **Hardware Manual**: Refer to printer manufacturer documentation

### Safety Reminders

- Always disconnect power when making wiring changes
- Use appropriate fuses or circuit protection
- Never exceed voltage or current ratings
- Test connections before final assembly
- Keep documentation updated with any modifications
