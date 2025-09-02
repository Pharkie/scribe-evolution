# Pipedream MQTT Bridge Integration Guide

## 🎯 **Purpose & Overview**

The Pipedream MQTT Bridge allows you to send messages to your Scribe thermal
printers from anywhere on the internet via HTTP requests. This integration acts
as a secure gateway that receives HTTP requests and forwards them as MQTT
messages to your printers.

### **What is Pipedream?**

[Pipedream](https://pipedream.com) is a serverless integration platform that
lets you connect APIs, databases, and services with simple code. It provides:

- **Serverless hosting** - No infrastructure to manage
- **Built-in HTTP endpoints** - Instant webhook URLs
- **Environment variables** - Secure credential storage
- **Real-time logs** - Debug and monitor workflows
- **Free tier** - Generous limits for hobby projects

### **Free Tier Limits (2025)**

- ✅ **10,000 workflow executions/month**
- ✅ **Unlimited workflows**
- ✅ **30-day log retention**
- ✅ **Built-in authentication**
- ✅ **No credit card required**

Perfect for typical Scribe printer usage patterns!

## 🔄 **Why Pipedream for HTTP-to-MQTT Bridge?**

### **The Self-Hosting Challenge**

Your Scribe printer's web interface is only accessible on your local network.
**Directly exposing your printer to the internet is dangerous and NOT
recommended** because it requires:

- **🚨 Firewall Holes** - Port forwarding exposes your home network to attacks
- **�️ Security Vulnerabilities** - Web interfaces aren't designed for internet
  exposure
- **🔧 Complex Setup** - Dynamic DNS, SSL certificates, router configuration
- **⚡ Always-On Requirements** - Your home network becomes a critical
  dependency
- **🌐 Attack Surface** - Tools like ngrok create security risks

**Instead, MQTT provides a secure solution** - your Scribe printer safely
receives messages from anywhere via a cloud MQTT broker, without exposing your
home network. You just need a way to send HTTP requests from web services to
that MQTT broker.

### **n8n is another option**

You could use n8n to act as an htto-mqtt bridge, but that needs:

- **💰 Always-On Hosting** - Self-hosted VPS or paid cloud hosting
  ($5-20+/month)
- **🔧 Infrastructure Management** - Server maintenance, updates, monitoring
- **📈 Resource Scaling** - Managing CPU/RAM as workflows grow
- **🛠️ Setup Complexity** - Docker, reverse proxies, database configuration

This is perfect if you already have infrastructure, but expensive just for a
simple HTTP-to-MQTT bridge.

### **Or you might try**

Other serverless platforms might work like Pipedream:

- **[Make.com](https://make.com)** - Visual automation with HTTP webhooks (1,000
  operations/month free)
- **[Zapier](https://zapier.com)** - Popular automation platform (100
  tasks/month free)
- **[n8n Cloud](https://n8n.io)** - Hosted n8n without infrastructure
  ($20+/month)

**Pipedream stands out** because it combines generous free limits, full coding
flexibility, and native MQTT library support - perfect for this specific use
case.

## 🚀 **Setup Guide**

### **Step 1: Create Pipedream Account**

1. Visit [pipedream.com](https://pipedream.com)
2. Click **"Sign up free"**
3. Create account
4. Verify email address

### **Step 2: Create New Workflow**

1. Click **"+ New"** → **"Workflow"**
2. Choose **"HTTP / Webhook"** as trigger
3. Name your workflow: `"Scribe MQTT Bridge"`

### **Step 3: Configure Pipedream Environment Variables**

Add these 4 secrets in **Account Settings** → **Environment Variables**:

| Variable        | Description                           | Example                |
| --------------- | ------------------------------------- | ---------------------- |
| `MQTT_host`     | Your MQTT broker hostname             | `your-mqtt-broker.com` |
| `MQTT_port`     | MQTT secure port                      | `8883`                 |
| `MQTT_username` | MQTT broker username - mark as secret | `scribe_user`          |
| `MQTT_password` | MQTT broker password - mark as secret | `your_secure_password` |

### **Step 4: Configure HTTP Trigger**

1. **Trigger Type**: HTTP / Webhook Requests
2. **HTTP Method**: POST
3. Set Event Data to HTTP Body Only
4. Set HTTP Response to "Return a custom response from your workflow"
5. **Authorization**: Custom token
6. **Token**: Invent a secure token (this is effectively a password to protect
   your web endpoint)
   ```
   Example: scribe_a8f3x9sdfsfw34f434334vsvdfvdfvdfverfdv344vb28282882828828090918912m2p7q1
   ```
7. **Save trigger** - note the webhook URL

### **Step 5: Add Code Step**

1. Click **"+"** → **"Add step"** → **"Run custom code"**
2. **Language**: Node.js
3. **Step name**: `publish_to_mqtt`
4. **Code**: Copy the complete [`pipedream.mjs`](../pipedream.mjs) content

### **Step 6: Test & Deploy**

1. Click **"Test"** in trigger section
2. Use the test payload (see below)
3. Check logs for successful MQTT publish
4. **Deploy** when working correctly

## 📋 **Usage Examples**

### **Test Payload (Pipedream Interface)**

```json
{
  "remote_printer": "Pharkie",
  "timestamp": "Mon 28 Jul 2025 18:20",
  "message": "Hello from Pipedream integration!",
  "sender": "pipedream_test"
}
```

### **cURL Request**

```bash
curl -X POST https://your-pipedream-webhook-url \
  -H "Authorization: Bearer scribe_webhook_2025_xyz123" \
  -H "Content-Type: application/json" \
  -d '{
    "remote_printer": "Pharkie",
    "timestamp": "2025-07-28 18:20:15",
    "message": "Test message from API",
    "sender": "api_client"
  }'
```

## 📊 **API Reference**

### **Endpoint**

```
POST https://your-pipedream-webhook-url
```

### **Headers**

```
Authorization: Bearer YOUR_CUSTOM_TOKEN
Content-Type: application/json
```

### **Request Body**

```json
{
  "remote_printer": "string", // Target printer name (required)
  "timestamp": "string", // Message timestamp (required)
  "message": "string", // Message content (required)
  "sender": "string" // Message sender ID (required)
}
```

### **Response Codes**

| Code  | Meaning      | Description                                    |
| ----- | ------------ | ---------------------------------------------- |
| `200` | Success      | Message published to MQTT                      |
| `400` | Bad Request  | Missing required fields or invalid JSON        |
| `401` | Unauthorized | Invalid or missing Bearer token                |
| `500` | Server Error | MQTT connection or environment variable issues |

### **Success Response**

```json
{
  "status": "Published",
  "topic": "scribe/Pharkie/inbox",
  "payload": "{\"message\":\"Hello!\",\"timestamp\":\"2025-07-28 18:20:15\",\"sender\":\"api\"}"
}
```

### **Error Response**

```json
{
  "error": "Missing required fields: remote_printer, message"
}
```

## 🔧 **Troubleshooting**

### **Common Issues**

**❌ 401 Unauthorized**

- Check Bearer token in Authorization header
- Verify token matches Pipedream trigger configuration

**❌ 400 Missing required fields**

- Ensure all 4 fields present: `remote_printer`, `timestamp`, `message`,
  `sender`
- Check JSON syntax is valid

**❌ 500 MQTT Error**

- Verify MQTT environment variables are set correctly
- Test MQTT broker connectivity
- Check MQTT credentials and permissions

**❌ No message received by printer**

- Verify `remote_printer` matches your printer's MQTT topic
- Check printer is connected to MQTT broker
- Review MQTT broker logs

### **Monitoring**

**View Logs:**

1. Go to your Pipedream workflow
2. Click **"Event History"**
3. Review execution logs for errors

**Test MQTT Connection:** Use an MQTT client to verify broker connectivity:

```bash
mosquitto_pub -h your-mqtt-broker.com -p 8883 -u username -P password \
  -t scribe/Pharkie/inbox \
  -m '{"message":"Test","timestamp":"2025-07-28","sender":"manual"}'
```

## 🔐 **Security Best Practices**

1. **Strong Bearer Token**
   - Use long, random tokens (20+ characters)
   - Include numbers, letters, and symbols
   - Rotate tokens periodically

2. **Environment Variables**
   - Never hardcode MQTT credentials
   - Use Pipedream's secure environment variables
   - Restrict MQTT user permissions to minimum required

3. **Network Security**
   - Use MQTTS (TLS encryption) only
   - Consider IP whitelisting on MQTT broker
   - Monitor unusual traffic patterns

4. **Access Control**
   - Limit Bearer token sharing
   - Document who has access
   - Revoke tokens when team members leave

For questions or issues, refer to the main [Scribe README](../README.md) or
create an issue in the repository.
