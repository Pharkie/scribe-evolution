# LED Configuration - Runtime Setup Guide

This guide explains how to configure LED settings in the Scribe thermal printer project using the web interface.

## Quick Start

1. **Enable LEDs** in `src/core/config.h`:
   ```cpp
   #define ENABLE_LEDS
   ```

2. **Connect your LED strip** to GPIO pin 4 (default) or your preferred pin

3. **Access the web interface** at `http://your-device-ip/settings.html`

4. **Navigate to LED Settings** section (💡 LED Settings button)

5. **Configure your LED strip**:
   - Select GPIO pin (0-10 for ESP32-C3)
   - Set LED count (number of LEDs in your strip)
   - Adjust brightness (0-255, recommend starting with 64)
   - Fine-tune effect parameters

6. **Save settings** - changes apply immediately without restart

## Configuration Parameters

### Hardware Settings
- **GPIO Pin**: Physical pin connected to LED strip data line (0-10)
- **LED Count**: Total number of LEDs in your strip (1-300)

### Performance Settings  
- **Brightness**: LED brightness from 0 (off) to 255 (maximum)
- **Refresh Rate**: Update frequency in Hz (10-120, recommend 60)

### Effect Settings
- **Fade Speed**: Transition speed for effects (1-255)
- **Twinkle Density**: Number of simultaneous twinkle stars (1-20)
- **Chase Speed**: Speed of chase effects (1-10)
- **Matrix Drops**: Number of matrix rain drops (1-15)

## Boot Effect

When the system starts successfully, it automatically triggers a 5-second rainbow effect to indicate LED system initialization.

## Runtime Updates

All LED configuration changes are:
- ✅ **Validated** - Input checking prevents invalid configurations
- ✅ **Persistent** - Settings saved to config.json automatically  
- ✅ **Immediate** - Applied without requiring system restart
- ✅ **Safe** - LED system gracefully reinitializes with new settings

## Troubleshooting

**LEDs not working after configuration change?**
- Check GPIO pin connection matches web interface setting
- Verify LED count matches your actual strip length
- Ensure LED strip is properly powered (5V, adequate current)
- Check console logs for initialization errors

**Settings not saving?**
- Verify you have write permissions to filesystem
- Check that config.json is not corrupted
- Review browser console for API errors

For detailed technical information, see [LED_EFFECTS.md](LED_EFFECTS.md).