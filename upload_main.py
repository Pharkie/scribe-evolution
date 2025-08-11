#!/usr/bin/env python3
# pylint: disable=undefined-variable
# Note: Import and env are provided by PlatformIO framework

Import("env")  # PlatformIO framework function
import os
import subprocess
import time
import glob

# Get the virtual environment Python path
VENV_PYTHON = os.path.join(os.getcwd(), ".venv", "bin", "python")
CURRENT_PYTHON = VENV_PYTHON if os.path.exists(VENV_PYTHON) else "python3"

# Try to import pyserial, but don't fail if it's not available
try:
    import serial

    HAS_PYSERIAL = True
except ImportError:
    HAS_PYSERIAL = False


def find_esp32_port():
    """Find ESP32 serial port on macOS"""
    possible_ports = [
        "/dev/cu.usbserial-*",
        "/dev/cu.SLAB_USBtoUART*",
        "/dev/cu.usbmodem*",
        "/dev/cu.wchusbserial*",
    ]

    for pattern in possible_ports:
        ports = glob.glob(pattern)
        if ports:
            print(f"🔍 Found potential ESP32 port: {ports[0]}")
            return ports[0]

    print("⚠️  No ESP32 port automatically detected")
    return None


def reset_esp32_connection(port=None):
    """Reset ESP32 connection and clear serial buffers"""
    if not HAS_PYSERIAL:
        print("⚠️  Skipping ESP32 reset - pyserial not available")
        return

    if not port:
        port = find_esp32_port()

    if not port:
        print("⚠️  Skipping ESP32 reset - no port found")
        return

    try:
        print(f"🔄 Resetting ESP32 connection on {port}...")

        # Multiple reset attempts for reliability
        for attempt in range(2):
            print(f"   Reset attempt {attempt + 1}/2...")
            ser = serial.Serial(port, 115200, timeout=1)

            # Clear any existing buffers
            ser.reset_input_buffer()
            ser.reset_output_buffer()

            # Hardware reset sequence
            ser.setDTR(False)  # Assert reset
            ser.setRTS(False)  # Also try RTS
            time.sleep(0.1)

            ser.setDTR(True)  # Release reset
            ser.setRTS(True)  # Release RTS
            time.sleep(0.1)

            ser.close()
            time.sleep(0.2)

        # Longer delay for ESP32 to fully stabilize
        time.sleep(1.0)
        print("✅ ESP32 connection reset completed")

    except (serial.SerialException, OSError) as e:
        print(f"⚠️  ESP32 reset failed (continuing anyway): {e}")


def kill_vscode_tasks():
    """Kill VS Code tasks that might be using serial ports"""
    try:
        print("🔄 Checking for VS Code tasks using serial ports...")

        # Find VS Code processes
        result = subprocess.run(
            ["pgrep", "-f", "Visual Studio Code"],
            capture_output=True,
            text=True,
            check=False,
        )

        if result.returncode == 0:
            print("   Found VS Code running")

            # More comprehensive process search
            ps_result = subprocess.run(
                ["ps", "-eo", "pid,ppid,command"],
                capture_output=True,
                text=True,
                check=False,
            )

            if ps_result.returncode == 0:
                killed_processes = []

                for line in ps_result.stdout.split("\n"):
                    # Look for processes that are likely to be using serial ports
                    if any(
                        keyword in line.lower()
                        for keyword in [
                            "platformio.*monitor",
                            "pio.*monitor",
                            "device monitor",
                            "serial monitor",
                            "usbmodem",
                            "cu.usb",
                            "ttyusb",
                            "platformio run.*monitor",
                        ]
                    ):
                        parts = line.strip().split()
                        if len(parts) >= 3 and parts[0].isdigit():
                            pid = parts[0]
                            if pid not in killed_processes:
                                print(
                                    f"   Killing serial task: {' '.join(parts[2:6])}... (PID: {pid})"
                                )
                                subprocess.run(["kill", "-TERM", pid], check=False)
                                killed_processes.append(pid)

                # Wait a moment, then force kill if necessary
                if killed_processes:
                    time.sleep(0.5)
                    for pid in killed_processes:
                        subprocess.run(["kill", "-9", pid], check=False)

        # Also check for any orphaned platformio processes
        pio_processes = subprocess.run(
            ["pgrep", "-f", "platformio"], capture_output=True, text=True, check=False
        )

        if pio_processes.returncode == 0:
            pids = pio_processes.stdout.strip().split("\n")
            for pid in pids:
                if pid:
                    print(f"   Killing orphaned PlatformIO process: {pid}")
                    subprocess.run(["kill", "-TERM", pid], check=False)
                    time.sleep(0.1)
                    subprocess.run(["kill", "-9", pid], check=False)

        print("✅ VS Code task cleanup completed")
        time.sleep(1.0)  # Give processes time to terminate

    except (subprocess.SubprocessError, OSError) as e:
        print(f"⚠️  VS Code task cleanup warning: {e}")


def kill_serial_processes():
    """Kill any processes that might be holding serial ports"""
    try:
        print("🔄 Checking for serial port conflicts...")

        # Kill any screen sessions on USB serial ports
        subprocess.run(
            ["pkill", "-f", "screen.*usbserial"],
            capture_output=True,
            check=False,  # Don't fail if no processes found
        )
        subprocess.run(
            ["pkill", "-f", "screen.*usbmodem"],
            capture_output=True,
            check=False,
        )

        # Kill any minicom sessions
        subprocess.run(["pkill", "-f", "minicom"], capture_output=True, check=False)

        # Kill any other PlatformIO monitor sessions
        subprocess.run(
            ["pkill", "-f", "pio.*monitor"], capture_output=True, check=False
        )
        subprocess.run(
            ["pkill", "-f", "platformio.*monitor"], capture_output=True, check=False
        )

        # Kill any VS Code serial monitor extensions
        subprocess.run(
            ["pkill", "-f", "code.*serial"], capture_output=True, check=False
        )

        # Kill any Python serial processes
        subprocess.run(
            ["pkill", "-f", "python.*serial"], capture_output=True, check=False
        )

        # Kill any processes using cu.usbmodem specifically
        subprocess.run(["pkill", "-f", "cu.usbmodem"], capture_output=True, check=False)

        print("✅ Serial port cleanup completed")
        time.sleep(0.5)  # Brief delay for processes to terminate

    except (subprocess.SubprocessError, OSError) as e:
        print(f"⚠️  Serial cleanup warning: {e}")


def reset_usb_on_macos():
    """Reset USB subsystem on macOS for better ESP32 reliability"""
    try:
        print("🔄 Resetting USB subsystem...")

        # Restart the USB daemon (requires admin rights, but may help)
        result = subprocess.run(
            ["sudo", "-n", "kextunload", "-b", "com.apple.driver.usb.IOUSBHostFamily"],
            capture_output=True,
            check=False,  # Don't fail if this doesn't work
        )

        if result.returncode == 0:
            time.sleep(0.5)
            subprocess.run(
                [
                    "sudo",
                    "-n",
                    "kextload",
                    "-b",
                    "com.apple.driver.usb.IOUSBHostFamily",
                ],
                capture_output=True,
                check=False,
            )
            print("✅ USB subsystem reset completed")
        else:
            print("⚠️  USB reset skipped (requires sudo)")

    except (subprocess.SubprocessError, OSError) as e:
        print(f"⚠️  USB reset warning: {e}")


def ensure_python_environment():
    """Ensure Python virtual environment is set up with required packages"""
    venv_path = os.path.join(os.getcwd(), ".venv")

    if not os.path.exists(venv_path):
        print("🐍 Creating Python virtual environment...")
        try:
            subprocess.run(
                ["python3", "-m", "venv", venv_path], check=True, capture_output=True
            )
            print("✅ Virtual environment created")
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to create virtual environment: {e}")
            return False

    # Install/update requirements
    requirements_file = os.path.join(os.getcwd(), "requirements.txt")
    if os.path.exists(requirements_file):
        print("📦 Installing Python dependencies...")
        try:
            subprocess.run(
                [CURRENT_PYTHON, "-m", "pip", "install", "-r", requirements_file],
                check=True,
                capture_output=True,
            )
            print("✅ Python dependencies installed")
        except subprocess.CalledProcessError as e:
            print(f"⚠️  Warning: Failed to install dependencies: {e}")

    return True


def upload_filesystem_and_firmware(source, target, env):
    """Upload filesystem first, then firmware with enhanced reliability"""
    # Note: source and target parameters required by PlatformIO callback signature
    _ = source, target  # Suppress unused parameter warnings

    print("🚀 Starting complete upload process...")

    # Step 0: Ensure Python environment is set up
    print("🐍 Checking Python environment...")
    if not ensure_python_environment():
        print("❌ Python environment setup failed!")
        env.Exit(1)

    # Step 0.5: Prepare serial connections
    print("🔧 Preparing ESP32 connection...")
    kill_vscode_tasks()  # Kill VS Code tasks first
    kill_serial_processes()
    reset_esp32_connection()

    # Step 1: Build Tailwind CSS
    print("🎨 Building Tailwind CSS...")
    try:
        # Run npm build-css command
        result = subprocess.run(
            ["npm", "run", "build-css"],
            cwd=os.getcwd(),
            check=True,
            capture_output=True,
            text=True,
        )
        print("✅ Tailwind CSS build completed successfully!")
        if result.stdout:
            print(f"   Output: {result.stdout.strip()}")
    except subprocess.CalledProcessError as e:
        print("❌ Tailwind CSS build failed!")
        print(f"   Error: {e.stderr}")
        env.Exit(1)
    except FileNotFoundError:
        print("❌ npm not found! Please ensure Node.js and npm are installed.")
        env.Exit(1)

    # Step 2: Build and minify JavaScript
    print("📦 Building and minifying JavaScript...")
    try:
        # Run npm build-js command
        result = subprocess.run(
            ["npm", "run", "build-js-prod"],
            cwd=os.getcwd(),  # Current working directory
            check=True,
            capture_output=True,
            text=True,
        )
        print("✅ JavaScript build completed successfully!")
        if result.stdout:
            print(f"   Output: {result.stdout.strip()}")
    except subprocess.CalledProcessError as e:
        print("❌ JavaScript build failed!")
        print(f"   Error: {e.stderr}")
        env.Exit(1)
    except FileNotFoundError:
        print("❌ npm not found! Please ensure Node.js and npm are installed.")
        env.Exit(1)

    # Step 3: Upload filesystem using PlatformIO
    print("📁 Uploading filesystem using PlatformIO...")
    fs_result = env.Execute("pio run --environment main --target uploadfs")
    if fs_result != 0:
        print("❌ Filesystem upload failed!")
        env.Exit(1)
    
    print("✅ Filesystem uploaded successfully!")

    # Step 4: Wait for ESP32 to boot and stabilize after filesystem upload
    print("⏳ Waiting for ESP32 to boot and stabilize after filesystem upload...")
    time.sleep(2.0)  # Allow ESP32 to fully boot and initialize filesystem

    # Step 5: Upload firmware and start monitoring
    print("💾 Uploading firmware and starting monitor...")
    fw_result = env.Execute(
        "pio run --environment main --target upload --target monitor"
    )
    if fw_result != 0:
        print("❌ Firmware upload failed!")
        env.Exit(1)

    print("✅ Complete upload finished and monitoring started!")
    print("🎯 Device should now boot without missing initial serial messages!")


# Add custom target
env.AddCustomTarget(  # pylint: disable=undefined-variable
    name="upload_main",
    dependencies=None,
    actions=[upload_filesystem_and_firmware],
    title="Python, CSS, Upload FS, Build & Upload Firmware, Monitor",
    description="Setup Python environment, build Tailwind CSS, then upload filesystem and firmware with enhanced workflow",
)
