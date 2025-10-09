# Quick Start Guide - Scribe Evolution Thermal Printer

## Step 1: Download Scribe Evolution Firmware

1. Download firmware for your board from the [Releases](https://github.com/Pharkie/scribe/releases) page
2. Extract the zip file to get `scribe-{environment}-complete.bin`.

## Step 2: Connect Your Board

1. Connect your ESP32 board to your computer via USB
2. Find your ESP32 port: Windows → Device Manager COM port; macOS → ls /dev/cu.\*; Linux → ls /dev/ttyUSB\* /dev/ttyACM. Replace below where the instructions say `/dev/cu.usbmodem1101`.

## Step 3: Flash the Firmware

You'll need `esptool` to flash the firmware. More info further down.

### Step 3a: Erase flash

```bash
esptool --port /dev/cu.usbmodem1101 erase-flash
```

### Step 3b: Flash firmware

```bash
esptool --port /dev/cu.usbmodem1101 --baud 460800 write-flash 0x0 scribe-esp32c3-prod-no-leds-complete.bin
```

Where the .bin is one of:

- `scribe-esp32c3-prod-no-leds-complete.bin` (ESP32-C3 without LEDs)
- `scribe-esp32c3-prod-complete.bin` (ESP32-C3 with LEDs)
- `scribe-esp32s3-mini-prod-complete.bin` (ESP32-S3 Mini with LEDs)
- `scribe-lolin32lite-no-leds-complete.bin` (Lolin32 Lite without LEDs)

## Step 4: Power up!

1. **Disconnect** your ESP32
2. Connect the thermal printer to your ESP32. Typically, GPIO21 (default; configurable) on the ESP32 connects to printer RX.
3. **Power up** your ESP32. Cross fingers 🤞

> 💡 **Power**: Use a separate 2.4A+ power supply for the thermal printer. Most USB ports can't power your thermal printer, due to the high power demands. With low current, you will get weird behaviour.

> Don't let the printer draw power from your board pins - always a separate power supply.

## Step 5: Setup

1. **Connect** to the setup network `Scribe-setup`. Password: `scribe123`
2. **Open browser** and visit `http://192.168.4.1` or wait for the Scribe Evolution setup page to appear.
3. **Configure your settings**:
   - WiFi
   - Device name (e.g., "Bob")
   - Timezone
4. **Choose "Save and Restart"**

Your Scribe Evolution will restart and connect to your WiFi network.

After connecting to your WiFi:

Open `http://scribe-XXXXXX.local` in a browser, where XXXXXX is your device name.

## Step 6: Set Up MQTT for Remote Printing (Optional)

### Setup MQTT to:

- **🖨️ Send privately to your friends** - connect multiple Scribe Evolution devices to print between them
- **🌍 Print from anywhere in the world**
- **🏠 Home automation integration** - have your smart home or Apple Shortcuts send text to your printer

### MQTT Server Setup

1. **Go to [HiveMQ Cloud](https://www.hivemq.com/)**
2. **Create account** (free tier) and **create a cluster**
3. **Note down**:
   - Server URL (e.g., `your-cluster.s1.eu.hivemq.cloud`)
   - Port (usually `8883`)
   - Username and password
4. **In your Scribe Evolution web interface** → Settings → MQTT:
   - Enable MQTT
   - Enter server details
   - Test connection
   - Save settings
   - Poke friends

## Need Help?

- **Documentation**: See the `/docs` folder in the repo
- **Issues**: Report problems on GitHub Issues (https://github.com/Pharkie/scribe/issues)
- **Community**: Join GitHub Discussions (https://github.com/Pharkie/scribe/discussions)

Welcome to the Scribe Evolution community! 🎉

## Where to get esptool

### Using pip (Recommended)

```bash
pip install esptool
```

### Using Homebrew (macOS)

```bash
brew install esptool
```

### Download esptool binary

Download from [espressif/esptool releases](https://github.com/espressif/esptool/releases)
