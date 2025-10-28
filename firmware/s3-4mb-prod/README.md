# Scribe Firmware Release

Built on: 2025-10-28 23:40:05

## Flashing Instructions:

**Step 1: Erase flash**

```bash
esptool --port /dev/cu.usbmodem1101 erase-flash
```

**Step 2: Flash firmware**

```bash
esptool --port /dev/cu.usbmodem1101 --baud 460800 write-flash 0x0 scribe-s3-4mb-prod-complete.bin
```

Replace `/dev/cu.usbmodem1101` with your actual port e.g. Windows: COM3, macOS: /dev/cu.usbserial-\*

Your Scribe Evolution will start a wifi network "Scribe-setup" with password "scribe123". Connect to it and navigate to http://192.168.4.1 to access the setup screen if it doesn't appear automatically.

## More details

Check out `/docs/quick-start.md`
