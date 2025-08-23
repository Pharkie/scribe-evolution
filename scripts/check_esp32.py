#!/usr/bin/env python3
"""
Quick ESP32-C3 connectivity checker - no build required!
This script quickly tests if the ESP32-C3 is connected and ready for upload.
"""

import subprocess
import sys
import os
from pathlib import Path


def check_port_exists(port="/dev/cu.usbmodem1201"):
    """Check if the serial port exists"""
    return os.path.exists(port)


def check_esp32_connection():
    """Quick ESP32 chip detection using esptool"""
    try:
        # Get PlatformIO's esptool path
        result = subprocess.run(
            [
                "pio",
                "pkg",
                "exec",
                "--package",
                "tool-esptoolpy",
                "--",
                "esptool.py",
                "--chip",
                "esp32c3",
                "--port",
                "/dev/cu.usbmodem1201",
                "--baud",
                "115200",
                "chip_id",
            ],
            capture_output=True,
            text=True,
            timeout=10,
        )

        if result.returncode == 0:
            # Extract chip info from output
            lines = result.stdout.split("\n")
            chip_info = [line for line in lines if "Chip is" in line or "MAC:" in line]
            return True, chip_info
        else:
            return False, [result.stderr.strip()]

    except subprocess.TimeoutExpired:
        return False, ["Connection timeout - ESP32 may need manual boot mode"]
    except Exception as e:
        return False, [f"Error: {str(e)}"]


def list_usb_devices():
    """List all USB serial devices"""
    try:
        result = subprocess.run(
            ["ls", "-la", "/dev/cu.usb*"], capture_output=True, text=True
        )
        if result.returncode == 0:
            return result.stdout.strip().split("\n")
        else:
            return ["No USB devices found"]
    except:
        return ["Could not list USB devices"]


def main():
    print("🔍 Quick ESP32-C3 Connectivity Check")
    print("=" * 40)

    # 1. Check if port exists
    port = "/dev/cu.usbmodem1201"
    if not check_port_exists(port):
        print(f"❌ Serial port {port} not found")
        print("\n📋 Available USB devices:")
        for device in list_usb_devices():
            print(f"   {device}")
        sys.exit(1)

    print(f"✅ Serial port {port} exists")

    # 2. Test ESP32 connection
    print("🔌 Testing ESP32-C3 connection...")
    connected, info = check_esp32_connection()

    if connected:
        print("✅ ESP32-C3 detected and ready!")
        for line in info:
            if line.strip():
                print(f"   {line}")
        print("\n🚀 Ready for upload!")
        sys.exit(0)
    else:
        print("❌ ESP32-C3 connection failed:")
        for line in info:
            if line.strip():
                print(f"   {line}")

        print("\n💡 Try these solutions:")
        print("   1. Put ESP32-C3 in boot mode (hold BOOT + press RESET)")
        print("   2. Try different USB cable")
        print("   3. Check if another process is using the port")
        sys.exit(1)


if __name__ == "__main__":
    main()
