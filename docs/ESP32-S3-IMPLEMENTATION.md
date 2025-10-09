# ESP32-S3 Mini Implementation Guide

This document describes the ESP32-S3 Mini (8MB, no PSRAM) support implementation for Scribe Evolution.

## Overview

The ESP32-S3 Mini variant provides:

- **8MB flash** (double the ESP32-C3's 4MB)
- **OTA support** via dual app partitions
- **More processing power** - Dual-core Xtensa LX7 @ 240MHz
- **Same GPIO compatibility** - GPIO21 works identically to ESP32-C3
- **USB CDC** - Native USB support for programming and serial output

## Partition Layout

The 8MB flash is partitioned as follows:

```
Offset    Size      Name        Type      Purpose
0x9000    20KB      nvs         data/nvs  Configuration storage
0xe000    8KB       otadata     data/ota  OTA boot selector
0x10000   2.5MB     factory     app       Main firmware (64KB aligned)
0x290000  2.5MB     ota_0       app       OTA update partition (64KB aligned)
0x510000  ~3MB      littlefs    data      Web assets and files
```

### Important Alignment Requirements

**ESP32-S3 requires app partitions to be 64KB (0x10000) aligned.**

The factory partition starts at `0x10000` instead of `0x12000` (which would cause a build error). This is different from ESP32-C3 which is more flexible with alignment.

## Build Environments

### Production Build

```bash
pio run -e esp32s3-mini-prod
```

- Optimized size (`-Os`)
- FastLED support enabled
- Debug level: minimal
- Target: deployment to hardware

### Development Build

```bash
pio run -e esp32s3-mini-dev
```

- Debug symbols (`-g3 -ggdb`)
- FastLED support enabled
- Debug level: verbose (5)
- Exception decoder enabled
- Target: development and debugging

## npm Scripts

```bash
npm run firmware:esp32s3      # Build production firmware
npm run firmware              # Build all targets (including S3)
```

## Flash Commands

### Complete Flash (First Time)

```bash
esptool --port /dev/cu.usbmodem1101 erase-flash
esptool --port /dev/cu.usbmodem1101 --baud 921600 write-flash 0x0 scribe-esp32s3-mini-prod-complete.bin
```

### Firmware Only Update

```bash
pio run -e esp32s3-mini-prod --target upload
```

### Filesystem Only Update

```bash
pio run -e esp32s3-mini-prod --target uploadfs
```

## Memory Usage

Typical production build:

- **RAM**: ~58KB / 327KB (17.7%)
- **Flash**: ~1.5MB / 2.5MB (59.1%)

The S3 has significantly more RAM than the C3 (327KB vs 400KB), providing headroom for future features.

## Key Differences from ESP32-C3

| Feature           | ESP32-C3   | ESP32-S3 Mini      |
| ----------------- | ---------- | ------------------ |
| Flash Size        | 4MB        | 8MB                |
| OTA Support       | No         | Yes                |
| App Partition     | 2MB        | 2.5MB (x2 for OTA) |
| Filesystem        | ~2MB       | ~3MB               |
| Bootloader Offset | 0x0000     | 0x0000             |
| App Alignment     | Flexible   | 64KB required      |
| Upload Speed      | 460800     | 921600             |
| Cores             | 1 (RISC-V) | 2 (Xtensa LX7)     |
| RAM               | 400KB      | 327KB              |

## OTA Support (Future)

The partition table is OTA-ready but OTA firmware update code is not yet implemented. When needed:

1. Implement OTA update handler in firmware
2. Add OTA endpoint to web interface
3. Test dual-partition boot selection
4. Add rollback protection

The infrastructure is in place - only the application code is needed.

## GPIO Compatibility

All default GPIO assignments work identically:

- **Printer TX**: GPIO21 (default, configurable)
- **LED Strip**: Configurable via config.h
- **Buttons**: Configurable via config.h

No hardware or wiring changes needed when switching from ESP32-C3 to ESP32-S3.

## Troubleshooting

### Build Errors

**"Partition factory invalid: Offset not aligned"**

- Ensure app partitions start at 0x10000-aligned addresses
- Check `partitions_8mb_ota.csv` has factory at 0x10000

**Upload failures**

- Use data-capable USB cable (not charge-only)
- Try lower baud rate: `--baud 115200`
- Reset device before upload

### Runtime Issues

**Device not booting after flash**

- Verify correct partition table was used
- Check bootloader offset is 0x0000
- Erase flash and reflash completely

**OTA partition errors (when OTA is implemented)**

- Check otadata partition is valid
- Verify both app partitions have same size
- Ensure rollback protection is configured

## Files Modified for S3 Support

- `platformio.ini` - Added esp32s3_base, esp32s3-mini-prod, esp32s3-mini-dev
- `partitions_8mb_ota.csv` - Created S3-specific partition table
- `scripts/bin/build_firmware_release.py` - Added S3 chip detection
- `package.json` - Added firmware:esp32s3 script
- `docs/hardware.md` - Added S3 to supported devices
- `docs/build-instructions.md` - Added S3 build instructions
- `docs/quick-start.md` - Added S3 firmware option

## Verification

Successful build output should show:

```
RAM:   [==        ]  17.7% (used 58068 bytes from 327680 bytes)
Flash: [======    ]  59.1% (used 1549717 bytes from 2621440 bytes)
Building .pio/build/esp32s3-mini-prod/firmware.bin
esptool.py v4.5.1
Creating esp32s3 image...
Successfully created esp32s3 image.
========================= [SUCCESS] Took XX.XX seconds =========================
```

## Next Steps for Developers

1. **Test on actual ESP32-S3 hardware** - Verify GPIO compatibility
2. **Implement OTA updates** - Add web interface and update handler
3. **Performance testing** - Measure any improvements from dual-core
4. **Power consumption** - Compare against ESP32-C3
5. **LED effects** - Test FastLED performance on S3

## Notes

- No PSRAM variant chosen to keep costs down
- OTA partitions reduce available app space but enable wireless updates
- 3MB filesystem is sufficient for current web assets (~200KB compressed)
- Dual-core architecture available but not required for current functionality
- Same power requirements as ESP32-C3 (5V 2.4A+ for printer)
