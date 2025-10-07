# Pipedream MQTT Bridge Integration Guide

Use Pipedream as a free HTTP→MQTT bridge to send messages to your Scribe from anywhere on the internet - perfect for Apple Shortcuts, web services, and automation tools.

> ⚠️ Set up HiveMQ first, see the [MQTT Integration Guide](mqtt-integration.md)

## Setup

### **Step 1: Create Environment Variables for MQTT Login**

Add your HiveMQ credentials (from the MQTT Integration Guide) to Pipedream:

1. Go to [pipedream.com](https://pipedream.com) and sign up (or log in)
2. Go to **Projects** (left sidebar)
3. Select **"New project"**
4. Name it `"Scribe Evolution"` and select **"Create Project"**

   ![New Project](assets/screenshots/mqtt/pipedream/pipedream-project1.png)

5. In your new project, go to **Variables** (left sidebar)
6. Select **"+ New variable"**
7. Add these 4 variables using your HiveMQ details:

| Variable        | Description                          | Where to find it                    | Mark as Secret? |
| --------------- | ------------------------------------ | ----------------------------------- | --------------- |
| `MQTT_host`     | Your HiveMQ cluster host             | HiveMQ cluster details page         | No              |
| `MQTT_port`     | MQTT secure port                     | Always `8883` for HiveMQ            | No              |
| `MQTT_username` | MQTT username you created in HiveMQ  | From "Create MQTT Credentials" step | **Yes**         |
| `MQTT_password` | MQTT password you copied from HiveMQ | From "Create MQTT Credentials" step | **Yes**         |

> 💡 **Tip**: These are the same credentials you entered in your Scribe device's MQTT settings.

![Project Variables](assets/screenshots/mqtt/pipedream/pipedream-project2.png)

### **Step 2: Create Workflow**

1. Select **"+ New"** → **"Workflow"**

   ![Select + New](assets/screenshots/mqtt/pipedream/pipedream-1.png)

2. Name your workflow: `"Scribe MQTT Bridge"`

![Name workflow](assets/screenshots/mqtt/pipedream/pipedream-2.png)

### Step 3: Create HTTP Trigger

Configure the webhook that will receive HTTP requests and forward them to MQTT.

1. **Add Trigger**: Type: "New HTTP / Webhook Requests"

   ![Create HTTP Trigger](assets/screenshots/mqtt/pipedream/pipedream-3.png)

2. Configure these settings:
   - **Event Data**: HTTP Body Only
   - **HTTP Response**: "Return a custom response from your workflow"
   authentication (protects your webhook from unauthorized access):
   - **Authorization**: Custom token
   - **Token**: Create a secure random token

   > 💡 Use a password manager to generate a long random string (20+ characters)
   >
   > Example: `scribe_a8f3x9sdfsfw34f434334vsvdfvdfvdfverfdv344vb28282882828828090918912m2p7q1`

4. **Save and continue**

![Configure HTTP Trigger](assets/screenshots/mqtt/pipedream/pipedream-4.png)

### Step 4: Add MQTT Publishing Code

Add the code that receives HTTP requests and publishes to your MQTT broker:

1. Select **"+"** to add the next step

![Add next step](assets/screenshots/mqtt/pipedream/pipedream-5.png)

2. Choose **"Run custom code"**

![Run custom code](assets/screenshots/mqtt/pipedream/pipedream-6.png)

3. **Language**: JS (default, Node.js) - not Python or Bash etc.
4. **Step name**: `publish_to_mqtt`
5. **Code**: Copy and paste the code from [`pipedream.mjs`](../pipedream.mjs)

![Paste code](assets/screenshots/mqtt/pipedream/pipedream-7.png)

> 💡 The code uses the environment variables you set up earlier

### Step 5: Done! Test.

Test that it works:

1. Select **"Test"** button in the trigger section
2. Use this test payload (replace "Pharkie" with your device name):
   ```json
   {
     "remote_printer": "Pharkie",
     "header": "MESSAGE",
     "body": "Hello from Pipedream integration!",
     "sender": "pipedream_test"
   }
   ```
3. Select **"Send Test Event"**
4. Check the execution logs - you should see:
   - ✅ "Published" status
   - Topic showing `scribe/YourName/print`
   - No errors

   ![Successful Test](assets/screenshots/mqtt/pipedream/pipedream-7.png)

5. **Your Scribe printer should print the test message!**
6. If successful, click **"Deploy"** to activate the workflow

## 📋 **Example MQTT messages**

### Test payload

```json
{
  "remote_printer": "Pharkie",
  "timestamp": "Mon 28 Jul 2025 18:20",
  "message": "Hello from Pipedream integration!",
  "sender": "pipedream_test"
}
```

### cURL request

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

- Verify all 4 MQTT environment variables are set correctly in Pipedream
- Double-check your HiveMQ credentials (username and password)
- Verify HiveMQ user has **Publish and Subscribe** permissions
- Test MQTT broker connectivity using a client tool
- Ensure port is `8883` (not `1883` or `8884`)

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

---

## What is Pipedream?

[Pipedream](https://pipedream.com) is a serverless integration platform that connects APIs, databases, and services with simple code. It provides:

- **Serverless hosting** - No infrastructure to manage
- **Built-in HTTP endpoints** - Instant webhook URLs
- **Environment variables** - Secure credential storage
- **Real-time logs** - Debug and monitor workflows
- **Free tier** - Generous limits for hobby projects

### Free Tier Limits (2025)

- ✅ **10,000 workflow executions/month**
- ✅ **Unlimited workflows**
- ✅ **30-day log retention**
- ✅ **Built-in authentication**
- ✅ **No credit card required**

Perfect for a typical Scribe Evolution printer.

### Why Use an HTTP-to-MQTT Bridge?

Your Scribe printer's web interface is only accessible on your local network. **Directly exposing your printer to the internet is dangerous and NOT recommended** because it requires:

- **🚨 Firewall Holes** - Port forwarding exposes your home network to attacks
- **🔧 Complex Setup** - Dynamic DNS, SSL certificates, router configuration
- **⚡ Always-On Requirements** - Your home network becomes a critical dependency
- **🌐 Attack Surface** - Tools like ngrok create security risks

**Instead, MQTT provides a secure solution** - your Scribe printer safely receives messages from anywhere via a cloud MQTT broker, without exposing your home network. You just need a way to send HTTP requests from web services to that MQTT broker.

### Alternatives to Pipedream

Other serverless platforms that might work in a similar way:

- **[Make.com](https://make.com)** - Visual automation with HTTP webhooks (1,000 operations/month free)
- **[Zapier](https://zapier.com)** - Popular automation platform (100 tasks/month free)
- **[n8n Cloud](https://n8n.io)** - Hosted n8n without infrastructure

---

For questions or issues, please create an issue in the repository.
