# Quick Start Guide - Scribe Evolution Thermal Printer

Get your Scribe Evolution thermal printer up and running in minutes! This guide walks you through flashing pre-built firmware and setting up your device.

## What You'll Need

### Required Hardware

- **ESP32-C3 Supermini** or **Lolin32 Lite** board and USB cable
- **Thermal printer**

### Optional Hardware (can be added later)

- **Hardware buttons** (for quick actions like printing jokes/riddles)
- **WS2812B LED strip** (for visual feedback)

> ⚠️ **Important**: LEDs and hardware buttons are optional. Your Scribe Evolution will work fine without them - you can control everything through the web interface.

## Step 1: Download Firmware

1. Go to the [Releases](https://github.com/Pharkie/scribe/releases) page
2. Download the latest firmware for your board:
   - `scribe-esp32c3-vX.X.X.zip` for ESP32-C3 Supermini
   - `scribe-lolin32lite-vX.X.X.zip` for Lolin32 Lite
3. Extract the zip file - you'll find:
   - `scribe-{board}-complete.bin` - **Complete firmware (single file - RECOMMENDED)**
   - `bootloader.bin`, `partitions.bin`, `firmware.bin`, `littlefs.bin` - Individual components (advanced)
   - `README.md` - Quick flashing instructions

## Step 2: Install Flashing Tools

You'll need `esptool` to flash the firmware. Choose your preferred method:

### Option A: Using pip (Recommended)

```bash
pip install esptool
```

### Option B: Using Homebrew (macOS)

```bash
brew install esptool
```

### Option C: Download binary

Download from [espressif/esptool releases](https://github.com/espressif/esptool/releases)

## Step 3: Connect Your Board

1. Connect your ESP32 board to your computer via USB
2. Find the serial port:

**Windows**: Check Device Manager → Ports (COM & LPT) → look for something like `COM3`

**macOS**:

```bash
ls /dev/cu.*
# Look for something like /dev/cu.usbserial-0001 or /dev/cu.usbmodem1101
```

**Linux**:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
# Usually /dev/ttyUSB0 or /dev/ttyACM0
```

## Step 4: Flash the Firmware

⚠️ **Important**: Erase the flash to avoid conflicts with existing firmware. This will erase all user settings/config.

Replace `/dev/ttyUSB0` with your actual port from Step 3.

### Step 1: Erase flash

```bash
esptool --port /dev/ttyUSB0 erase-flash
```

### Step 2: Flash complete firmware

```bash
esptool --port /dev/ttyUSB0 --baud 460800 write-flash 0x0 scribe-{board}-complete.bin
```

Replace `{board}` with either `esp32c3` or `lolin32lite` depending on your hardware.

**Examples:**

- ESP32-C3: `esptool --port /dev/ttyUSB0 --baud 460800 write-flash 0x0 scribe-esp32c3-complete.bin`
- Lolin32: `esptool --port /dev/ttyUSB0 --baud 460800 write-flash 0x0 scribe-lolin32lite-complete.bin`

### Windows Users

Use `COM3` (or your port) instead of `/dev/ttyUSB0`:

```cmd
esptool --port COM3 erase-flash
esptool --port COM3 --baud 460800 write-flash 0x0 scribe-esp32c3-complete.bin
```

### macOS Users

Use your specific port (like `/dev/cu.usbserial-0001`) instead of `/dev/ttyUSB0`:

```bash
esptool --port /dev/cu.usbserial-0001 erase-flash
esptool --port /dev/cu.usbserial-0001 --baud 460800 write-flash 0x0 scribe-esp32c3-complete.bin
```

## Step 5: Connect Your Thermal Printer

1. **Power off** your ESP32
2. Connect the thermal printer to your ESP32:
   - **ESP32-C3 Supermini**: TX pin is GPIO21 (default)
   - **Lolin32 Lite**: TX pin is GPIO17 (default)
   - Connect printer RX to ESP32 TX
   - Connect printer GND to ESP32 GND
   - Connect printer VCC to 5V power supply (NOT ESP32 3.3V)
3. **Power on** your ESP32

> 💡 **Tip**: Thermal printers need 5V power with good amperage. Don't try to power them from the board pins - use a separate power supply.

## Step 6: Initial Setup (Access Point Mode)

When your Scribe boots for the first time, it creates its own WiFi network:

1. **Look for WiFi network**: `Scribe-Setup`
2. **Connect** to this network from your phone/computer
3. **Open browser** and go to: `http://192.168.4.1` or wait for it load the WiFi settings screen
4. **Configure your settings**:
   - WiFi credentials (your home network)
   - Device name (e.g., "Kitchen Scribe")
   - MQTT settings (optional, see Step 7)
   - OpenAI API key (optional, for AI-generated content)
5. **Click "Save and Restart"**

Your Scribe Evolution will restart and connect to your home WiFi network.

## Step 7: Find Your Device

After connecting to your WiFi:

1. **Check your router's admin panel** for connected devices, or
2. **Use mDNS**: `http://scribe-XXXXXX.local` (where XXXXXX matches your device name), or
3. **Scan your network**: Use tools like `nmap` or network scanner apps

Once you find your Scribe Evolution's IP address, bookmark it for easy access!

## Step 8: (Optional) Set Up MQTT for Remote Printing

**MQTT is optional** - your Scribe Evolution works perfectly fine without it using just the web interface. However, setting up a free private MQTT server unlocks powerful remote capabilities:

### What MQTT Enables:

- **📱 Remote printing** via Apple Shortcuts, Zapier, or any automation tool
- **🌍 Print from anywhere in the world** - send a message to your home printer from your office
- **🖨️ Multi-printer networks** - connect multiple Scribe Evolution devices to share messages
- **🏠 Home automation integration** - have your smart home send notifications to specific printers

### Free MQTT Server Setup

1. **Go to [HiveMQ Cloud](https://www.hivemq.com/)** (free tier available)
2. **Create account** and **create a cluster**
3. **Note down**:
   - Server URL (e.g., `your-cluster.s1.eu.hivemq.cloud`)
   - Port (usually `8883` for secure connection)
   - Username and password
4. **In your Scribe Evolution web interface** → Settings → MQTT:
   - Enter server details
   - Test connection
   - Save settings

### Example Use Cases:

- **Apple Shortcuts**: Create a shortcut that sends your current location to your kitchen printer when you leave work
- **Multi-printer setup**: Send a grocery list to your kitchen printer AND your spouse's office printer simultaneously
- **Remote messaging**: Send a "good morning" note to your partner's bedside printer from across the world
- **Smart home**: Have your security system print alerts to your front hall printer

Each Scribe Evolution can both send messages to other printers AND receive messages from anywhere on the same MQTT server.

## Step 9: Start Printing!

Your Scribe Evolution is ready! Try these features:

### Web Interface

- **Home page**: Quick actions (Joke, Riddle, Quote, News, etc.)
- **Settings page**: Configure everything
- **Diagnostics page**: View logs and system info

### Physical Buttons (if connected)

- **Short press**: Quick action (joke, riddle, etc.)
- **Long press**: Different action (configurable)
- **LED feedback**: Visual confirmation (if LED strip connected)

### MQTT Control (if configured)

Send JSON messages to your device's topic:

```json
{
  "message": "Hello from the internet!",
  "sender": "Your Name"
}
```

## Troubleshooting

### Device Won't Boot

- Check power supply (thermal printer needs 5V)
- Verify wiring connections
- Try re-flashing firmware with `erase_flash` first

### Can't Connect to Setup Network

- Wait 60 seconds after power-on for setup mode
- Look for network `Scribe-Setup-XXXXXX`
- Try restarting the device (power cycle)

### Printer Not Responding

- Check thermal printer power (needs 5V, not 3.3V)
- Verify TX/RX wiring (ESP32 TX → Printer RX)
- Test with a different printer if available
- Check Settings → Printer for correct GPIO pin

### Web Interface Not Loading

- Verify device IP address
- Try `http://scribe-XXXXXX.local` instead of IP
- Check if device is connected to your WiFi
- Try accessing via setup mode (Step 6)

### MQTT Not Working

- Test connection in Settings → MQTT
- Verify server URL, port, credentials
- Check firewall settings
- Try a different MQTT broker

## Advanced Usage

### Custom Content

- **Memos**: Store up to 4 custom messages with placeholder support (`{TIME}`, `{DATE}`)
- **Unbidden Ink**: AI-generated content on schedule
- **Custom Actions**: Use hardware buttons for personalized quick actions

### Hardware Buttons

GPIO pins can be configured for buttons with customizable actions:

- Print specific content types
- Send MQTT messages
- Trigger LED effects
- Custom memo printing

### LED Effects

WS2812B LED strips can provide visual feedback:

- Rainbow patterns
- Pulse effects
- Chase animations
- Status indicators

## Advanced: Individual Component Flashing

If you need to flash individual components (for development or troubleshooting), you can use the separate binary files.

### ESP32-C3 Supermini

```bash
# Erase flash first
esptool --port /dev/ttyUSB0 erase-flash

# Flash individual components (no OTA, so no boot_app0.bin needed)
esptool --port /dev/ttyUSB0 --baud 460800 write-flash \
    0x0000 bootloader.bin \
    0x8000 partitions.bin \
    0x10000 firmware.bin \
    0x2B0000 littlefs.bin
```

### Lolin32 Lite

```bash
# Erase flash first
esptool --port /dev/ttyUSB0 erase-flash

# Flash individual components (no OTA, so no boot_app0.bin needed)
esptool --port /dev/ttyUSB0 --baud 460800 write-flash \
    0x1000 bootloader.bin \
    0x8000 partitions.bin \
    0x10000 firmware.bin \
    0x2B0000 littlefs.bin
```

> **Note**: Individual component flashing is only recommended for advanced users. The single merged binary is easier and less error-prone. ESP32-C3 and Lolin32 have different bootloader addresses (0x0000 vs 0x1000).

## Need Help?

- **Documentation**: Check `/docs/` folder for technical details
- **Issues**: Report problems on [GitHub Issues](https://github.com/YOUR_REPO/issues)
- **Community**: Join discussions in [GitHub Discussions](https://github.com/YOUR_REPO/discussions)

## What's Next?

- **Customize your prompts** for AI-generated content
- **Set up scheduled printing** with Unbidden Ink
- **Add hardware buttons** for quick access to favorite actions
- **Connect multiple Scribe Evolution devices** via MQTT for a printer network
- **Integrate with home automation** using MQTT or HTTP API

Welcome to the Scribe Evolution community! 🎉📰
