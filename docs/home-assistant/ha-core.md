# Home Assistant Core (Manual venv) – MQTT Bridge Setup

This guide is for **Home Assistant Core** installations with **system-installed Mosquitto**.

[← Back to setup guide selection](README.md)

---

## Prerequisites

- Home Assistant Core (manual Python venv installation)
- Mosquitto installed as system service (`sudo apt install mosquitto`)
- SSH or terminal access to your system
- Your **HiveMQ Cloud** credentials - see [MQTT Integration Guide](../mqtt-integration.md)

---

## Step 1: Create Bridge Configuration File

Create the bridge configuration file:

```bash
sudo nano /etc/mosquitto/conf.d/bridge.conf
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

> 💡 **Note:** Adjust `bridge_cafile` path if your CA certificates are located elsewhere (e.g., `/etc/ssl/cert.pem` on some systems).

Save and exit (`Ctrl+X`, `Y`, `Enter`).

---

## Step 2: Configure Mosquitto to Include Bridge File

Edit your main Mosquitto configuration:

```bash
sudo nano /etc/mosquitto/mosquitto.conf
```

Add this line (if not already present):

```conf
# Include all config files in conf.d
include_dir /etc/mosquitto/conf.d
```

Save and exit.

---

## Step 3: Restart Mosquitto Service

Restart Mosquitto to apply the configuration:

```bash
sudo systemctl restart mosquitto
```

Check the service status:

```bash
sudo systemctl status mosquitto
```

You should see `active (running)`.

---

## Step 4: Verify Connection

Check Mosquitto logs:

```bash
sudo journalctl -u mosquitto -f
```

Look for entries indicating the bridge is connecting to HiveMQ:

```
Loading config file /etc/mosquitto/conf.d/bridge.conf
Connecting bridge hivemq (...)
```

Press `Ctrl+C` to exit log viewer.

---

## Step 5: Test the Bridge

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

- Check HiveMQ credentials are correct
- Verify HiveMQ Cloud allows connections from your IP
- Check firewall allows outbound port 8883

**Permission denied?**

- Ensure `/etc/mosquitto/conf.d/bridge.conf` is readable by mosquitto user:
  ```bash
  sudo chown mosquitto:mosquitto /etc/mosquitto/conf.d/bridge.conf
  sudo chmod 640 /etc/mosquitto/conf.d/bridge.conf
  ```

---

## Related Documentation

- [MQTT Integration Guide](../mqtt-integration.md) - Set up HiveMQ Cloud
- [Pipedream Integration](../pipedream-integration.md) - HTTP→MQTT bridge
- [Apple Shortcuts Integration](../apple-shortcuts.md) - Print from iOS/macOS

[← Back to setup guide selection](README.md)
