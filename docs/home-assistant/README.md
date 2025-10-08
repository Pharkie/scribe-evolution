# Home Assistant Integration via MQTT Bridge

This guide shows you how to integrate your Scribe Evolution printer with Home Assistant.

Create an MQTT bridge between Home Assistant's Mosquitto broker and HiveMQ Cloud. Then you can **print to Scribe Evolution from Home Assistant**, including from automations, service calls, scripts etc.

```
Home Assistant (Mosquitto) ←→ MQTT Bridge ←→ HiveMQ Cloud ←→ Scribe Evolution
```

- Turned off the light? Print a receipt. Collect ten for a prize.
- Heating too high? Log that on paper so you can show your landlord.
- Fed the cat? Print a record for the vet's files.

## Choose Your Setup Guide

Select the guide that matches your Home Assistant installation:

### [Home Assistant OS / Supervised](ha-supervised.md)

**You have this if:**

- Running Home Assistant OS (e.g., Home Assistant Green, Yellow, or VM)
- Using the Mosquitto Broker add-on from the Add-on Store
- Managing add-ons through **Settings** → **Add-ons**

**Setup method:** Configure via add-on UI

---

### [Home Assistant Core (Manual venv)](ha-core.md)

**You have this if:**

- Installed Home Assistant Core manually using Python virtual environment
- Running Mosquitto as a system service (installed via `apt`, `yum`, etc.)
- Managing Mosquitto via `systemctl`

**Setup method:** Edit system config files

---

### [Home Assistant Container (Docker)](ha-container.md)

**You have this if:**

- Running Home Assistant in Docker (not Home Assistant OS)
- Running Mosquitto as a separate Docker container (`eclipse-mosquitto` image)
- Managing containers via `docker` or `docker-compose`

**Setup method:** Mount config files, edit container configs

---

## Still Not Sure?

Check which installation type you have:

```bash
# If this shows "Home Assistant OS" or "Home Assistant Supervised"
ha core info

# If Home Assistant command isn't available, check if you installed via:
# - Python venv → Core
# - Docker container → Container
```

## Prerequisites (All Methods)

Before starting, you need:

- Your own **HiveMQ Cloud** instance - see [MQTT Integration Guide](../mqtt-integration.md). You'll need your MQTT hostname, port, username and password
- Ability to edit configuration files (requirements vary by installation type)

---

## Related Documentation

- [MQTT Integration Guide](../mqtt-integration.md) - Set up HiveMQ Cloud
- [Pipedream Integration](../pipedream-integration.md) - HTTP→MQTT bridge for external services
- [Apple Shortcuts Integration](../apple-shortcuts.md) - Print from iOS/macOS

---

For questions or issues, please create an issue in the repository.
