# Apple Shortcuts Integration

This document explains how to integrate Apple Shortcuts with your Scribe Evolution thermal printer using HTTP-to-MQTT bridge services.

## Overview

Apple Shortcuts can trigger your Scribe Evolution remotely by sending HTTP requests to a bridge (e.g., Pipedream) that converts HTTP→MQTT.

- Send messages to your printer from anywhere with internet access
- Trigger printing from Siri voice commands
- Automate printing based on iOS shortcuts and automations
- Print from any location without being on your local network

## Architecture

```
Apple Shortcuts → HTTP Request → Bridge Service → MQTT Broker → Scribe Evolution Printer
```

### Why a Bridge Service?

Apple Shortcuts can't send MQTT messages directly, and exposing your Scribe Evolution printer to the internet creates security risks. A bridge service provides a secure intermediary that:

- Receives HTTP requests from Apple Shortcuts
- Converts them to MQTT messages
- Forwards messages to your printer via a secure cloud MQTT broker
- Eliminates the need to open firewall ports or expose your home network

## Bridge Options

### Pipedream (Recommended)

**Why Pipedream?**

- **Free tier**: 10,000 executions/month (generous for typical usage)
- **No infrastructure**: Fully serverless
- **Easy setup**: Simple HTTP webhook configuration
- **Secure**: Built-in authentication and environment variables
- **Reliable**: Enterprise-grade uptime

**📖 [Complete Pipedream Setup Guide](pipedream-integration.md)**

### Alternative Bridge Services

**n8n / Make.com / Zapier** – viable alternatives if you already use them.

**Make.com (formerly Integromat)**

- Visual automation interface
- 1,000 operations/month on free tier
- Good for non-technical users
- More limited free tier

**Zapier**

- Popular automation platform
- Only 100 tasks/month on free tier
- Easy setup but limited free usage
- More expensive for regular use

## Shortcuts Setup (basic)

Once you have a bridge service configured, create shortcuts to send messages:

### Basic “Print Text” Shortcut

1. **Add "Ask for Input" action**:
   - Input Type: Text
   - Prompt: "What would you like to print?"

2. **Add "Get Contents of URL" action**:
   - URL: `https://your-bridge-webhook-url`
   - Method: POST
   - Headers:
     ```
     Authorization: Bearer YOUR_SECRET_TOKEN
     Content-Type: application/json
     ```
   - Request Body:
     ```json
     {
       "remote_printer": "YourPrinterName",
       "header": "MESSAGE",
       "body": "[Provided Input]",
       "sender": "Shortcuts"
     }
     ```

3. **Add "Show Notification" action**:
   - Title: "Sent to Printer"
   - Body: "Your message was sent successfully"

### Voice Activation

1. Create the basic shortcut above
2. **Record Siri phrase**: "Print this" or "Send to printer"
3. **Test with Siri**: "Hey Siri, print this"

### Quick Message Shortcuts

Create multiple shortcuts for common messages:

**"Print Shopping List"**:

```json
{
  "remote_printer": "KitchenPrinter",
  "header": "SHOPPING LIST",
  "body": "- Milk\n- Bread\n- Eggs\n- Apples",
  "sender": "Shortcuts"
}
```

**"Print Daily Reminder"**:

```json
{
  "remote_printer": "BedroomPrinter",
  "header": "DAILY REMINDER",
  "body": "Good morning! Today's goals:\n- Exercise 30min\n- Read 20min\n- Call family",
  "sender": "Shortcuts"
}
```

### Advanced: Location-Based Automation

1. **Create automation**: Settings → Shortcuts → Automation → Create Personal Automation
2. **Trigger**: When I arrive/leave a location
3. **Actions**: Run your printing shortcut
4. **Example**: Print "Welcome home!" when arriving home

### Advanced: Time-Based Automation

1. **Create automation**: Time of Day trigger
2. **Set schedule**: Daily at specific time
3. **Actions**: Print daily quote, reminder, or schedule
4. **Example**: Print morning motivation at 7:00 AM

## Message Format Details

### Required Fields

All messages to the bridge service must include:

```json
{
  "remote_printer": "string", // Target printer name (must match config)
  "header": "string", // Message header like MESSAGE, JOKE, etc.
  "body": "string", // Message content to print
  "sender": "string" // Identifier for message source
}
```

### Field Specifications

**remote_printer**:

- Must match the printer name configured in your Scribe Evolution's device settings
- Case-sensitive
- Examples: "Alice", "Bob", "OfficeMain"

**header**:

- Message type header printed at the top
- Examples: "MESSAGE", "JOKE", "RIDDLE", "QUOTE", "MEMO 1"-"MEMO 4"
- Used to categorize the type of message

**body**:

- UTF-8 text content to print
- Line breaks supported with `\n`
- Maximum length depends on printer paper width
- Special characters automatically mapped for thermal printing

**sender**:

- Identifier for who/what sent the message
- Appended to header as "[header] from [sender]"
- Examples: "Shortcuts", "Siri", "iOS", "HomeKit"
- Helps with message tracking and accountability

### Dynamic Content

Use Shortcuts variables for dynamic content:

**User Input**:

```json
{
  "body": "[Provided Input from Ask for Input action]"
}
```

**Location Data**:

```json
{
  "body": "Location update: [Current Address]"
}
```

**Current Date/Time** (optional timestamp field):

```json
{
  "timestamp": "[Current Date formatted as 'yyyy-MM-dd HH:mm:ss']"
}
```

## Troubleshooting

### Common Issues

**❌ Message not printing**:

- Verify printer name matches configuration exactly
- Check MQTT broker connectivity
- Confirm printer is powered on and has paper

**❌ HTTP request fails**:

- Check webhook URL is correct
- Verify authentication token
- Ensure Content-Type header is set to application/json

**❌ Invalid JSON error**:

- Validate JSON syntax using a JSON validator
- Check for missing commas or quotes
- Ensure all required fields are present

**❌ Authentication errors**:

- Verify Bearer token in Authorization header
- Check token matches bridge service configuration
- Ensure token doesn't contain extra spaces

### Testing

**Test from Shortcuts**:

1. Create a simple test shortcut with fixed values
2. Run manually to verify basic functionality
3. Check bridge service logs for error messages
4. Gradually add dynamic content and complexity

**Test bridge service directly**:
Use a tool like Postman or curl to test the bridge endpoint:

```bash
curl -X POST https://your-bridge-webhook-url \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "remote_printer": "TestPrinter",
    "header": "MESSAGE",
    "body": "Test message from curl",
    "sender": "Debug"
  }'
```

**Monitor MQTT traffic**:
Use an MQTT client to monitor messages:

```bash
mosquitto_sub -h your-mqtt-broker.com -p 8883 \
  -u username -P password \
  -t "scribe-evolution/print/+"
```

## Security Considerations

### Token Security

- Use long, random authentication tokens
- Don't share tokens in screenshots or documentation
- Rotate tokens periodically
- Store tokens securely in Shortcuts

### Network Security

- Bridge services use HTTPS for encrypted communication
- MQTT broker connections use TLS encryption
- No direct exposure of your home network required
- Printer remains on local network only

### Access Control

- Limit who has access to authentication tokens
- Monitor bridge service logs for unusual activity
- Consider IP restrictions if supported by bridge service
- Revoke tokens immediately if compromised

## Best Practices

### Shortcut Organization

- Use descriptive names for shortcuts
- Group related shortcuts in folders
- Add icons and colors for easy identification
- Document shortcut purposes in comments

### Message Content

- Keep messages concise for thermal paper width
- Use line breaks (`\n`) for readability
- Include timestamps for context
- Avoid very long messages that waste paper

### Error Handling

- Add notification actions to confirm successful sending
- Include error handling for failed requests
- Test shortcuts regularly to ensure they still work
- Have fallback options for critical messages

### Usage Monitoring

- Monitor bridge service usage against free tier limits
- Track which shortcuts are used most frequently
- Review and clean up unused shortcuts regularly
- Consider usage patterns when designing automations
