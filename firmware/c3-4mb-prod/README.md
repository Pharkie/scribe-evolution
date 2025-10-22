# Scribe Firmware Release

Built on: 2025-10-22 23:42:25

## Flashing Instructions:

**Step 1: Erase flash**

```bash
esptool --port /dev/cu.usbmodem1101 erase-flash
```

**Step 2: Flash firmware**

```bash
esptool --port /dev/cu.usbmodem1101 --baud 460800 write-flash 0x0 scribe-c3-4mb-prod-complete.bin
```

Replace `/dev/cu.usbmodem1101` with your actual port e.g. Windows: COM3, macOS: /dev/cu.usbserial-\*

Your Scribe Evolution will start a wifi network "Scribe-setup" with password "scribe123" and go from there.

## More details

Check out `/docs/quick-start.md`
