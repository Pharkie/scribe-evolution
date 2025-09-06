# Scribe Firmware Release

Built on: 2025-09-06 23:15:17

## Files in this directory:

- `scribe-lolin32lite-no-leds-complete.bin` - Complete firmware as a single file
- `bootloader.bin`, `partitions.bin`, `firmware.bin`, `littlefs.bin` - Individual Flash partitions (advanced)

## Flashing Instructions:

**Step 1: Erase flash**

```bash
esptool --port /dev/cu.usbmodem1101 erase-flash
```

**Step 2: Flash firmware**

```bash
esptool --port /dev/cu.usbmodem1101 --baud 460800 write-flash 0x0 scribe-lolin32lite-no-leds-complete.bin
```

Replace `/dev/cu.usbmodem1101` with your actual port (e.g. Windows: COM3, macOS: /dev/cu.usbserial-\*).

That's it! Your Scribe is ready to use.

## Need more help?

See `/docs/quick-start.md`
