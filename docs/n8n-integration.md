# n8n Integration Guide

This document explains how to use n8n as an HTTP-to-MQTT bridge for integrating external services with your Scribe thermal printer.

## Overview

n8n provides a powerful workflow automation platform that can bridge the gap between HTTP-based services (like Apple Shortcuts, IFTTT, Zapier) and your MQTT-enabled Scribe printer. This eliminates the need to expose your local network to the internet while still allowing remote printing capabilities.

## What is n8n?

**n8n** is an open-source workflow automation tool that lets you connect different services and APIs through visual workflows. For Scribe integration, n8n acts as a translator that:

- Receives HTTP POST requests (webhooks) from external services
- Processes and validates the incoming data
- Converts HTTP requests to MQTT messages
- Forwards messages to your Scribe printer via MQTT broker

## Why Use n8n for Scribe Integration?

### Advantages

**Universal Integration**:
- Works with Apple Shortcuts, IFTTT, Zapier, and any service that can send HTTP POST requests
- No need to modify existing automation workflows
- Standardized webhook interface

**Security Benefits**:
- No local networking required - external services reach printer through MQTT
- Avoids firewall holes, port forwarding, or ngrok tunnels
- Uses encrypted MQTT connections (TLS) instead of exposing printer to internet
- No direct access to your home network

**Flexibility**:
- Can process, format, or validate messages before printing
- Support for complex workflows and conditional logic
- Data transformation and enrichment capabilities
- Multiple input sources can feed into single printer

**Reliability**:
- Cloud-hosted solution with high uptime
- Built-in error handling and retry mechanisms
- Message queuing and delivery confirmation
- Monitoring and alerting capabilities

### Compared to Alternatives

**vs. Pipedream**:
- More complex setup but more powerful workflow capabilities
- Better for users who need advanced automation logic
- Requires hosting but offers more customization

**vs. Direct HTTP Integration**:
- Avoids security risks of exposing printer to internet
- No need for dynamic DNS or SSL certificate management
- Works from anywhere without VPN requirements

## Setup Guide

### Step 1: Choose Hosting Platform

**Recommended Options**:

**Fly.io** (Recommended for beginners):
- Free tier available
- Simple deployment process
- Good documentation and support
- Automatic SSL certificates

**Railway**:
- Simple deployment with GitHub integration
- Free tier available
- One-click n8n deployment templates

**DigitalOcean**:
- More control over server configuration
- Requires basic server management knowledge
- $5-10/month pricing
- Better for production deployments

**Self-hosted**:
- Maximum control and customization
- Requires server administration skills
- Need to handle updates and security
- Can be most cost-effective for multiple workflows

### Step 2: Deploy n8n

**Fly.io Deployment**:
```bash
# Install flyctl
curl -L https://fly.io/install.sh | sh

# Deploy n8n
flyctl launch --name your-n8n-instance
```

**Railway Deployment**:
1. Connect GitHub account to Railway
2. Deploy from n8n template
3. Configure environment variables
4. Deploy and get your URL

**Docker Self-hosting**:
```bash
docker run -it --rm \
  --name n8n \
  -p 5678:5678 \
  -e WEBHOOK_URL="https://your-domain.com" \
  -v ~/.n8n:/home/node/.n8n \
  n8nio/n8n
```

### Step 3: Create Webhook to MQTT Workflow

**Workflow Structure**:
```
HTTP Webhook → Data Validation → Message Processing → MQTT Publisher
```

**1. Add Webhook Node**:
- **Node Type**: "Webhook"
- **Method**: POST
- **Path**: `/webhook/scribe` (or your preferred path)
- **Authentication**: None (or Basic Auth for security)
- **Response**: Return confirmation message

**2. Add Data Processing Node** (optional):
- **Node Type**: "Code" or "Set"
- **Purpose**: Validate input, format message, extract data
- **Example**: Extract message text, validate printer name

**3. Add MQTT Node**:
- **Node Type**: "MQTT"
- **Broker**: Your MQTT broker hostname (e.g., HiveMQ Cloud)
- **Port**: 8883 (for TLS) or 1883 (for plain)
- **Topic**: Dynamic based on input or fixed
- **Message**: JSON formatted for Scribe

### Step 4: Configure MQTT Connection

**MQTT Node Settings**:
```
Protocol: mqtt
Host: your-broker.hivemq.cloud
Port: 8883
Username: your-mqtt-username
Password: your-mqtt-password
Client ID: n8n-scribe-bridge
SSL/TLS: Enable
```

**Topic Configuration**:
```javascript
// Dynamic topic based on input
scribe/{{$json.printer_name}}/inbox

// Or fixed topic for single printer
scribe/MyPrinter/inbox
```

**Message Format**:
```json
{
  "message": "{{$json.message}}",
  "timestamp": "{{$now.format('YYYY-MM-DD HH:mm:ss')}}",
  "sender": "n8n_webhook"
}
```

### Step 5: Test the Workflow

**Test Webhook**:
```bash
curl -X POST https://your-n8n.fly.dev/webhook/scribe \
  -H "Content-Type: application/json" \
  -d '{
    "message": "Test message from n8n",
    "printer_name": "MyPrinter"
  }'
```

**Expected Flow**:
1. n8n receives webhook POST request
2. Data is validated and processed
3. MQTT message is published to broker
4. Scribe printer receives message and prints
5. HTTP response confirms successful processing

## Advanced Configuration

### Message Validation

**Input Validation Node**:
```javascript
// Validate required fields
if (!$json.message || !$json.printer_name) {
  throw new Error('Missing required fields: message, printer_name');
}

// Validate message length
if ($json.message.length > 500) {
  throw new Error('Message too long (max 500 characters)');
}

// Sanitize input
const cleanMessage = $json.message.replace(/[^\x00-\x7F]/g, "");
return [{json: {
  message: cleanMessage,
  printer_name: $json.printer_name,
  timestamp: new Date().toISOString()
}}];
```

### Multi-Printer Support

**Dynamic Routing**:
```javascript
// Route to different printers based on input
const printerMap = {
  'kitchen': 'scribe/kitchen/inbox',
  'office': 'scribe/office/inbox', 
  'bedroom': 'scribe/bedroom/inbox'
};

const topic = printerMap[$json.printer_name] || 'scribe/default/inbox';
return [{json: {...$json, topic: topic}}];
```

### Error Handling

**Error Handling Node**:
```javascript
try {
  // Process message
  const result = processMessage($json);
  return [{json: result}];
} catch (error) {
  // Log error and return user-friendly message
  console.error('Processing failed:', error);
  return [{json: {
    error: true,
    message: 'Failed to process message',
    details: error.message
  }}];
}
```

### Authentication

**Basic Auth Setup**:
```
Username: scribe_api
Password: your-secure-password
```

**API Key Validation**:
```javascript
// Validate API key from header or body
const apiKey = $request.headers['x-api-key'] || $json.api_key;
if (apiKey !== 'your-secret-api-key') {
  throw new Error('Invalid API key');
}
```

## Integration Examples

### Apple Shortcuts

**Shortcut Configuration**:
1. **Add "Ask for Input" action**
2. **Add "Get Contents of URL" action**:
   - URL: `https://your-n8n.fly.dev/webhook/scribe`
   - Method: POST
   - Headers: `Content-Type: application/json`
   - Request Body:
   ```json
   {
     "message": "[Provided Input]",
     "printer_name": "kitchen"
   }
   ```

### IFTTT Integration

**IFTTT Webhook Action**:
- URL: `https://your-n8n.fly.dev/webhook/scribe`
- Method: POST
- Content Type: `application/json`
- Body:
```json
{
  "message": "{{TriggerValue}}",
  "printer_name": "office",
  "source": "ifttt"
}
```

### Zapier Integration

**Zapier Webhook Step**:
1. Choose "Webhooks by Zapier" app
2. Select "POST" action
3. Configure URL and payload
4. Test the webhook integration

## Monitoring and Debugging

### n8n Execution Logs

**Viewing Logs**:
1. Access n8n web interface
2. Navigate to "Executions" tab
3. View detailed execution logs
4. Check for errors or failed steps

**Common Issues**:
- MQTT connection failures
- Invalid JSON format
- Authentication errors
- Network connectivity problems

### MQTT Monitoring

**Monitor MQTT Traffic**:
```bash
# Subscribe to all scribe topics
mosquitto_sub -h your-broker.com -p 8883 -u username -P password -t "scribe/+/inbox"

# Monitor specific printer
mosquitto_sub -h your-broker.com -p 8883 -u username -P password -t "scribe/MyPrinter/inbox"
```

**Debug MQTT Messages**:
```javascript
// Add debug node to log MQTT messages
console.log('Publishing to MQTT:', {
  topic: $json.topic,
  message: $json.message,
  timestamp: new Date().toISOString()
});
```

## Security Considerations

### Network Security

- Always use HTTPS for webhook endpoints
- Use TLS encryption for MQTT connections (port 8883)
- Consider IP whitelisting if supported by hosting platform
- Monitor for unusual traffic patterns or abuse

### Authentication

- Implement API key validation for production use
- Use strong, randomly generated API keys
- Rotate keys periodically
- Log authentication failures

### Data Validation

- Validate all input data before processing
- Sanitize text content to prevent injection attacks
- Limit message size to prevent resource exhaustion
- Rate limit requests to prevent abuse

## Troubleshooting

### Common Issues

**Webhook not receiving requests**:
- Check URL is correct and accessible
- Verify HTTP method (POST) is configured
- Test with curl or Postman
- Check firewall and network settings

**MQTT publishing fails**:
- Verify MQTT broker credentials
- Check network connectivity to broker
- Ensure topic permissions are correct
- Test MQTT connection separately

**Message format errors**:
- Validate JSON syntax
- Check required fields are present
- Verify data types match expectations
- Test with simple message first

**n8n workflow errors**:
- Check execution logs in n8n interface
- Verify node configurations
- Test each node individually
- Check for JavaScript errors in code nodes

### Debug Tips

1. **Enable verbose logging** in n8n workflow
2. **Test components separately** (webhook, MQTT, processing)
3. **Use simple test data** to isolate issues
4. **Monitor MQTT broker logs** for delivery confirmation
5. **Check Scribe printer logs** for message reception

## Cost Considerations

### Hosting Costs

**Free Tier Options**:
- Fly.io: Limited monthly hours
- Railway: $5/month after free tier
- Heroku: Free tier discontinued

**Paid Options**:
- DigitalOcean: $5-10/month
- AWS/GCP: Variable based on usage
- Self-hosted: Server costs only

### Usage Optimization

- Design efficient workflows to minimize execution time
- Use conditional logic to avoid unnecessary processing
- Implement caching where appropriate
- Monitor usage to stay within free tier limits

## Conclusion

n8n provides a powerful and flexible solution for integrating external services with your Scribe printer. While setup is more complex than serverless alternatives like Pipedream, it offers greater flexibility for users who need advanced automation capabilities or want to host their own infrastructure.

The combination of HTTP webhooks and MQTT messaging provides a secure, reliable way to enable remote printing without exposing your local network to security risks.