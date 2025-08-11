## 📝 v0.1.0 – Alpha Release

This is the first public alpha release of firmware for the **Scribe ESP32-C3
Thermal Printer** project.

Expect rough edges — feedback welcome.

---

### 🔧 Flashing Instructions

To flash the firmware to your ESP32-C3, you'll need
[`esptool.py`](https://github.com/espressif/esptool):

```bash
pip install esptool
```

Then run the following command, replacing the port with your device's actual
serial port:

```bash
esptool.py --chip esp32c3 --port /dev/tty.usbserial-XXXX --baud 460800 write_flash -z 0x0 firmware.bin
```

> ⚠️ On macOS, the port is likely `/dev/tty.usbserial-*` or
> `/dev/cu.usbmodem*`.  
> 🧠 Hold the `BOOT` button when connecting if flashing fails.

---

### 🚧 Known Limitations

- Built specifically for ESP32-C3 (may not run on other boards)

---

### 💬 About this Project

**Scribe** is an experimental thermal printer interface powered by an ESP32-C3
microcontroller.

This release makes it easier for users to try it without compiling the code
themselves.
