# Home Assistant Container (Docker) – MQTT Bridge Setup

This guide is for **Home Assistant Container** (Docker) with a **separate Mosquitto container** (`eclipse-mosquitto` image).

> ⚠️ **This guide is currently untested.** If you successfully (or unsuccessfully) follow this guide, please share feedback via GitHub issues. Contributions welcome!

[← Back to setup guide selection](README.md)

---

## Prerequisites

- Home Assistant running in Docker
- Mosquitto running as a separate Docker container (version 1.6 or 2.x)
- Docker or Docker Compose installed
- Terminal/SSH access to your Docker host
- Your **HiveMQ Cloud** credentials - see [MQTT Integration Guide](../mqtt-integration.md)

> 💡 **Version Note:** This guide covers both Mosquitto 1.6 and 2.x. The main difference is authentication defaults—Mosquitto 2.0+ requires explicit authentication configuration (covered in Step 3a).

---

## Step 1: Create Host Folders

Create directories on your Docker host:

```bash
mkdir -p /opt/mosquitto/config /opt/mosquitto/data /opt/mosquitto/log
```

**Set proper ownership for Mosquitto user:**

```bash
sudo chown -R 1883:1883 /opt/mosquitto
```

> ⚠️ **Important:** The Mosquitto container runs as user ID 1883. Files must be owned by this user or the container won't be able to read configs or write logs.

> 💡 Adjust paths if your setup uses a different location (e.g., `~/mosquitto/`). If using TLS certificates (see Step 4a), set permissions after copying cert files.

---

## Step 2: Create Bridge Configuration

Create `/opt/mosquitto/config/50-bridge.conf`:

```bash
nano /opt/mosquitto/config/50-bridge.conf
```

> 💡 **Why "50-"?** Numeric prefixes ensure config files load in the correct order. The main config includes all `.conf` files in the directory.

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
# Syntax: topic <pattern> <direction> <qos> [local_prefix] [remote_prefix]
# direction: in (remote→local), out (local→remote), both (bidirectional)
topic scribe/# both 0
```

> ⚠️ **About `cleansession`:**
>
> - `true` = Fresh start on every reconnect (useful for testing, avoids stale subscriptions)
> - `false` = Resume previous session (better for production, maintains QoS state)

> 💡 **Note:** Adjust `bridge_cafile` path if your CA certificates are located elsewhere in the container (e.g., `/etc/ssl/cert.pem` on some systems). See Step 4a for custom certificate setup.

Save and exit.

---

## Step 3: Create Mosquitto Configuration

### Main Configuration File

Create `/opt/mosquitto/config/mosquitto.conf`:

```bash
nano /opt/mosquitto/config/mosquitto.conf
```

**For Mosquitto 2.0+**, paste this configuration:

```conf
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
log_dest stdout
socket_domain ipv4

# Mosquitto 2.0+ defaults to allow_anonymous false
# We'll configure authentication in Step 3a
listener 1883

include_dir /mosquitto/config
```

**For Mosquitto 1.6**, paste this configuration:

```conf
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
log_dest stdout
socket_domain ipv4

# Mosquitto 1.6 defaults to allow_anonymous true
# Explicitly set for clarity (configure auth in Step 3a if needed)
listener 1883
allow_anonymous true

include_dir /mosquitto/config
```

> ⚠️ **Important Version Difference:**
>
> - **Mosquitto 1.6**: Defaults to `allow_anonymous true` (insecure but works immediately)
> - **Mosquitto 2.0+**: Defaults to `allow_anonymous false` (secure but requires authentication setup)

> 💡 **Why `include_dir`?** This directive loads all `.conf` files from the directory. This is why we mount the entire `/opt/mosquitto/config` directory, not individual config files. The bridge config (`50-bridge.conf`) will be loaded automatically.

> 💡 **About `socket_domain ipv4`:** Forces IPv4 connections. Some Docker environments have IPv6 networking issues that can prevent bridge connections.

Save and exit.

---

## Step 3a: Configure Authentication (Required for Mosquitto 2.0+)

**If you're using Mosquitto 1.6 and want to skip authentication**, you can jump to Step 4.

**For Mosquitto 2.0+ or for securing Mosquitto 1.6**, configure authentication for local clients (Home Assistant):

### Option 1: Quick Start (Testing Only)

Allow anonymous access temporarily for testing:

```bash
nano /opt/mosquitto/config/mosquitto.conf
```

Add or modify the line:

```conf
allow_anonymous true
```

> ⚠️ **Security Warning:** This allows anyone on your network to connect to your MQTT broker without credentials. Only use for testing, never in production.

### Option 2: Password Authentication (Recommended)

Create a password file with username/password for Home Assistant:

```bash
# Create password file (run on host, not in container)
docker run -it --rm -v /opt/mosquitto/config:/mosquitto/config eclipse-mosquitto:2 mosquitto_passwd -c /mosquitto/config/password.txt homeassistant
```

You'll be prompted to enter a password twice. This creates `/opt/mosquitto/config/password.txt`.

**Set correct permissions:**

```bash
sudo chown 1883:1883 /opt/mosquitto/config/password.txt
sudo chmod 600 /opt/mosquitto/config/password.txt
```

**Update mosquitto.conf** to enable password authentication:

```bash
nano /opt/mosquitto/config/mosquitto.conf
```

Add these lines before `include_dir`:

```conf
allow_anonymous false
password_file /mosquitto/config/password.txt
```

> 💡 **Adding more users:** Omit the `-c` flag to add users (don't recreate the file):
>
> ```bash
> docker run -it --rm -v /opt/mosquitto/config:/mosquitto/config eclipse-mosquitto:2 mosquitto_passwd /mosquitto/config/password.txt another_user
> ```

**Important:** You'll need to update your Home Assistant configuration (Step 6) with these credentials.

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
      # - "8883:8883"   # Uncomment if using TLS
      # - "9001:9001"   # Uncomment if using WebSockets
    volumes:
      - /opt/mosquitto/config:/mosquitto/config # Mount directory, not individual files!
      - /opt/mosquitto/data:/mosquitto/data
      - /opt/mosquitto/log:/mosquitto/log
    restart: unless-stopped
```

> ⚠️ **Critical:** Always mount the entire `/mosquitto/config` directory, not individual config files. The `include_dir` directive requires directory access to load all `.conf` files.

Then run:

```bash
docker-compose up -d mosquitto
```

---

## Step 4a: TLS Configuration (Optional)

If you need custom TLS certificates for local listeners (not the bridge—HiveMQ Cloud TLS is already configured in Step 2):

### 1. Copy Certificates to Config Directory

```bash
# Copy your certificate files to the config directory
sudo cp /path/to/your/server.crt /opt/mosquitto/config/
sudo cp /path/to/your/server.key /opt/mosquitto/config/
sudo cp /path/to/your/ca.crt /opt/mosquitto/config/

# Set correct permissions
sudo chown 1883:1883 /opt/mosquitto/config/*.crt /opt/mosquitto/config/*.key
sudo chmod 644 /opt/mosquitto/config/*.crt
sudo chmod 600 /opt/mosquitto/config/*.key
```

> ⚠️ **Important:** Private keys (`.key` files) must be readable only by the Mosquitto user (mode 600).

### 2. Add TLS Listener to Main Config

Edit `/opt/mosquitto/config/mosquitto.conf` and add a TLS listener:

```conf
# Add after the existing listener 1883 section
listener 8883
cafile /mosquitto/config/ca.crt
certfile /mosquitto/config/server.crt
keyfile /mosquitto/config/server.key
require_certificate false
```

### 3. Update Bridge Config for Custom Certs (if needed)

If your bridge needs to use custom CA certificates instead of system defaults, edit `/opt/mosquitto/config/50-bridge.conf`:

```conf
# Replace this line:
bridge_cafile /etc/ssl/certs/ca-certificates.crt

# With:
bridge_cafile /mosquitto/config/ca.crt
```

### 4. Restart Container

```bash
docker restart mosquitto
```

> 💡 **HiveMQ Cloud Note:** HiveMQ Cloud already uses TLS (port 8883) with system CA certificates. This section is only needed if you're adding additional TLS listeners or using custom certificates.

---

## Step 5: Verify Bridge Connection

**After any configuration change, follow this checklist:**

### 1. Restart Container

```bash
# For Docker:
docker restart mosquitto

# For Docker Compose:
docker-compose restart mosquitto
```

### 2. Check Logs for Bridge Connection

```bash
# For Docker:
docker logs -f mosquitto

# For Docker Compose:
docker-compose logs -f mosquitto
```

**Look for these key messages:**

```
Loading config file /mosquitto/config/50-bridge.conf
Connecting bridge hivemq (a08something.s1.eu.hivemq.cloud:8883)
```

**Success indicators:**

- `Connecting bridge hivemq` appears without errors
- No "Connection refused" or "Authentication failed" messages
- Bridge doesn't repeatedly disconnect/reconnect

> **Note:** A successful connection may not produce a "connected" log entry. If you see "Connecting bridge" without subsequent errors, the bridge is working.

### 3. Test from Scribe Evolution

Send a test message from your Scribe printer to verify end-to-end connectivity.

### 4. Verify on HiveMQ Cloud Console

Log into HiveMQ Cloud console and check for:

- Active bridge connection from your HA broker
- Messages appearing in the `scribe/#` topic

Press `Ctrl+C` to exit log view.

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
          topic: "scribe-evolution/print/YourPrinterName"
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
2. Set topic: `scribe-evolution/print/Krists`
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

### Broker Won't Start (Mosquitto 2.0+)

**Problem:** Container starts then immediately exits, or shows authentication errors in logs.

**Most common cause:** Missing or misconfigured authentication on Mosquitto 2.0+.

**Check logs:**

```bash
docker logs mosquitto
```

**Look for errors like:**

- `Error: Unable to open password file`
- `Error: Authentication required`
- Config syntax errors

**Solutions:**

1. **If you see password file errors:**
   - Either create the password file (Step 3a Option 2)
   - Or enable anonymous access (Step 3a Option 1)

2. **If you see config syntax errors:**
   - Check `mosquitto.conf` for typos
   - Ensure `include_dir /mosquitto/config` has correct path
   - Verify file ownership: `ls -la /opt/mosquitto/config`

3. **Quick fix for testing (Mosquitto 2.0):**
   ```bash
   nano /opt/mosquitto/config/mosquitto.conf
   # Add this line before include_dir:
   allow_anonymous true
   ```
   Then restart: `docker restart mosquitto`

### Configuration Changes Not Applied

**Problem:** Made changes to config files but bridge still not working.

**Solution checklist:**

1. **Verify file ownership:**

   ```bash
   sudo chown -R 1883:1883 /opt/mosquitto/config
   ls -la /opt/mosquitto/config
   ```

   All config files must be owned by user/group 1883.

2. **Check file naming:**

   ```bash
   ls /opt/mosquitto/config/*.conf
   ```

   Bridge config should be named `50-bridge.conf` (or similar numeric prefix) to ensure proper load order.

3. **Restart container:**

   ```bash
   docker restart mosquitto
   ```

4. **Check logs for syntax errors:**
   ```bash
   docker logs mosquitto | grep -i error
   ```

### Bridge Not Connecting

**Problem:** Bridge configuration exists but not connecting to HiveMQ Cloud.

**Check these common issues:**

1. **Credentials are case-sensitive:**
   - Verify username and password in `50-bridge.conf` match HiveMQ Cloud exactly
   - No extra spaces or quotes around values

2. **Network and firewall:**
   - Check firewall allows outbound port 8883
   - Verify container has network access: `docker exec mosquitto ping -c 3 8.8.8.8`
   - Check HiveMQ Cloud allows connections from your IP (console → Access Management)

3. **Missing critical settings in main config:**
   - Verify `socket_domain ipv4` is in `mosquitto.conf`
   - Ensure `allow_anonymous false` is in root config (not bridge config)
   - Confirm `include_dir /mosquitto/config` is present

4. **TLS certificate issues:**
   - Default path `/etc/ssl/certs/ca-certificates.crt` may not exist
   - Try system CA paths: `/etc/ssl/cert.pem` or `/etc/ssl/certs/`
   - Or mount custom certs (see Step 4a)

5. **Check bridge logs:**
   ```bash
   docker logs mosquitto | grep -i bridge
   ```
   Look for "Connecting bridge hivemq" followed by connection errors.

### Home Assistant Can't Connect to Mosquitto

**Problem:** HA can't reach local Mosquitto broker.

**Version-specific issues:**

**Mosquitto 2.0+ (most common):**

- **Authentication required by default.** Check logs for "not authorised" messages:
  ```bash
  docker logs mosquitto | grep -i "not authorised"
  ```
- **Solution:** Either configure password authentication (Step 3a Option 2) or temporarily allow anonymous access (Step 3a Option 1)
- Verify password file exists if using authentication:
  ```bash
  ls -la /opt/mosquitto/config/password.txt
  ```

**Mosquitto 1.6:**

- Authentication is optional (defaults to anonymous allowed)
- If you configured authentication, ensure password file exists and HA credentials match

**General solutions:**

- Check they're on the same Docker network, or use host IP address
- Verify port 1883 is exposed: `docker ps | grep mosquitto`
- Check Mosquitto logs for connection attempts:
  ```bash
  docker logs -f mosquitto
  ```
- Verify Home Assistant configuration matches your authentication setup (Step 6)

### Permission Issues with Volumes

**Problem:** Container can't read config or write logs.

**Solution:**

```bash
sudo chown -R 1883:1883 /opt/mosquitto
sudo chmod 755 /opt/mosquitto/config /opt/mosquitto/data /opt/mosquitto/log

# If using TLS certificates:
sudo chmod 644 /opt/mosquitto/config/*.crt
sudo chmod 600 /opt/mosquitto/config/*.key
```

**Verify permissions:**

```bash
ls -la /opt/mosquitto/config
```

All files should show `1883:1883` ownership.

---

## Related Documentation

- [MQTT Integration Guide](../mqtt-integration.md) - Set up HiveMQ Cloud
- [Pipedream Integration](../pipedream-integration.md) - HTTP→MQTT bridge
- [Apple Shortcuts Integration](../apple-shortcuts.md) - Print from iOS/macOS

[← Back to setup guide selection](README.md)
