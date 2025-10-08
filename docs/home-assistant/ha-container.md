# Home Assistant Container (Docker) – MQTT Bridge Setup

This guide is for **Home Assistant Container** (Docker) with a **separate Mosquitto container** (`eclipse-mosquitto` image).

[← Back to setup guide selection](README.md)

---

## Prerequisites

- Home Assistant running in Docker
- Mosquitto running as a separate Docker container
- Docker or Docker Compose installed
- Terminal/SSH access to your Docker host
- Your **HiveMQ Cloud** credentials - see [MQTT Integration Guide](../mqtt-integration.md)

---

## Step 1: Create Host Folders

Create directories on your Docker host:

```bash
mkdir -p /opt/mosquitto/config /opt/mosquitto/data /opt/mosquitto/log
```

> 💡 Adjust paths if your setup uses a different location (e.g., `~/mosquitto/`).

---

## Step 2: Create Mosquitto Configuration

### Main Configuration File

Create `/opt/mosquitto/config/mosquitto.conf`:

```bash
nano /opt/mosquitto/config/mosquitto.conf
```

Paste this configuration:

```conf
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log

listener 1883
allow_anonymous false

# Include bridge configuration
include_dir /mosquitto/config
```

Save and exit.

---

## Step 3: Create Bridge Configuration

Create `/opt/mosquitto/config/bridge.conf`:

```bash
nano /opt/mosquitto/config/bridge.conf
```

Paste the following and **replace the placeholders**:

```conf
connection hivemq
addresses a08something.s1.eu.hivemq.cloud:8883
remote_username <your-hivemq-user>
remote_password <your-hivemq-pass>

# IDs
local_clientid ha-mosq
remote_clientid ha-bridge

# Protocol + session
bridge_protocol_version mqttv311
try_private false
cleansession true
keepalive_interval 60
restart_timeout 5 30
notifications false

# TLS (HiveMQ Cloud requires it)
bridge_cafile /etc/ssl/certs/ca-certificates.crt
bridge_insecure false
bridge_tls_version tlsv1.2

# Topic routing
# Mirror Scribe messages both ways
topic scribe/# both 0
```

> 💡 **Note:** Adjust `bridge_cafile` path if your CA certificates are located elsewhere in the container (e.g., `/etc/ssl/cert.pem` on some systems).

Save and exit.

---

## Step 4: Run Mosquitto Container

### Using Docker Run

```bash
docker run -d --name mosquitto \
  -p 1883:1883 \
  -v /opt/mosquitto/config:/mosquitto/config \
  -v /opt/mosquitto/data:/mosquitto/data \
  -v /opt/mosquitto/log:/mosquitto/log \
  eclipse-mosquitto:2
```

### Using Docker Compose

If you're using Docker Compose, add this to your `docker-compose.yml`:

```yaml
services:
  mosquitto:
    image: eclipse-mosquitto:2
    container_name: mosquitto
    ports:
      - "1883:1883"
    volumes:
      - /opt/mosquitto/config:/mosquitto/config
      - /opt/mosquitto/data:/mosquitto/data
      - /opt/mosquitto/log:/mosquitto/log
    restart: unless-stopped
```

Then run:

```bash
docker-compose up -d mosquitto
```

---

## Step 5: Verify Connection

Check Mosquitto container logs:

```bash
# For Docker:
docker logs mosquitto

# For Docker Compose:
docker-compose logs mosquitto
```

Look for:

```
Loading config file /mosquitto/config/bridge.conf
Connecting bridge hivemq (...)
```

> **Note:** Connection success may not generate a log entry. If you see "Connecting bridge" without errors, the bridge is likely working.

### Follow logs in real-time:

```bash
# For Docker:
docker logs -f mosquitto

# For Docker Compose:
docker-compose logs -f mosquitto
```

Press `Ctrl+C` to exit.

---

## Step 6: Configure Home Assistant

Ensure Home Assistant is configured to connect to your Mosquitto container.

In your Home Assistant `configuration.yaml`:

```yaml
mqtt:
  broker: mosquitto # Or IP address if not using Docker network
  port: 1883
  username: your-mqtt-user
  password: your-mqtt-password
```

> 💡 If using Docker networks, use the container name (`mosquitto`). Otherwise, use the host IP.

Restart Home Assistant.

---

## Step 7: Test the Bridge

### Test HiveMQ → Home Assistant

1. In Home Assistant, go to **Settings** → **Devices & Services** → **MQTT** → **Configure**
2. Select **Listen to a topic**
3. Enter topic: `scribe/#`
4. Select **Start listening**
5. Publish a test message to your HiveMQ broker:
   ```json
   {
     "header": "KITTY UPDATE",
     "body": "Subject has completed zoomies and entered loaf mode. Purring intensity: medium-high.",
     "sender": "Feline Observation Unit"
   }
   ```
6. You should see the message appear in Home Assistant

### Test Home Assistant → HiveMQ

1. On the same screen, select **Publish a packet**
2. Topic: `scribe/inbox`
3. Payload:
   ```json
   {
     "header": "MESSAGE",
     "body": "Remember to pick up milk, bread, eggs and kitty treats.",
     "sender": "Harold"
   }
   ```
4. Select **Publish**
5. Check HiveMQ Cloud console or MQTT Explorer to confirm the message arrived

---

## Done!

Now you can send messages to your Scribe Evolution printer from Home Assistant automations.

### Example Automation

```yaml
automation:
  - alias: "Print morning greeting to Scribe Evolution"
    trigger:
      - platform: time
        at: "07:00:00"
    action:
      - service: mqtt.publish
        data:
          topic: "scribe/YourPrinterName/print"
          payload: >
            {
              "header": "GOOD MORNING",
              "body": "Today is {{ now().strftime('%A, %B %d') }}. Have a great day!",
              "sender": "HomeAssistant"
            }
```

### Developer Tools: Trigger a Print

Manually publish to your Scribe topic using **Developer Tools** → **Actions**:

1. Type "mqtt" in search bar and choose "Publish" action
2. Set topic: `scribe/Krists/print`
3. Tick **Payload** then paste:
   ```json
   {
     "header": "URGENT TRANSMISSION",
     "body": "Captain, the coffee reserves are critically low. Recommend immediate resupply before morale collapses.",
     "sender": "Starship Pantry Control"
   }
   ```
4. Click **Perform action**

---

## Troubleshooting

**Bridge not connecting?**

- Check HiveMQ credentials are correct in `bridge.conf`
- Verify HiveMQ Cloud allows connections from your IP
- Check firewall allows outbound port 8883
- Verify container has network access: `docker exec mosquitto ping -c 3 8.8.8.8`

**Home Assistant can't connect to Mosquitto?**

- Check they're on the same Docker network, or use host IP
- Verify port 1883 is exposed: `docker ps | grep mosquitto`
- Check Mosquitto logs for connection attempts

**Permission issues with volumes?**

```bash
sudo chown -R 1883:1883 /opt/mosquitto/data /opt/mosquitto/log
sudo chmod -R 755 /opt/mosquitto
```

---

## Restart Container

If you need to restart Mosquitto after config changes:

```bash
# For Docker:
docker restart mosquitto

# For Docker Compose:
docker-compose restart mosquitto
```

---

## Related Documentation

- [MQTT Integration Guide](../mqtt-integration.md) - Set up HiveMQ Cloud
- [Pipedream Integration](../pipedream-integration.md) - HTTP→MQTT bridge
- [Apple Shortcuts Integration](../apple-shortcuts.md) - Print from iOS/macOS

[← Back to setup guide selection](README.md)
