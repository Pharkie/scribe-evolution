# Scribe Firmware Release
    
Built on: 2025-08-27 16:31:36

## Files in this directory:
- `scribe-esp32c3-complete.bin` - Complete firmware as a single file
- `bootloader.bin`, `partitions.bin`, `firmware.bin`, `littlefs.bin` - Individual Flash partitions (advanced)

## Flashing Instructions:

**Step 1: Erase flash**
```bash
esptool --port /dev/ttyUSB0 erase-flash
```

**Step 2: Flash firmware**  
```bash
esptool --port /dev/ttyUSB0 --baud 460800 write-flash 0x0 scribe-esp32c3-complete.bin
```

Replace `/dev/ttyUSB0` with your actual port (e.g. Windows: COM3, macOS: /dev/cu.usbserial-*).

That's it! Your Scribe is ready to use.

## Need more help?
See `/docs/quick-start.md`
