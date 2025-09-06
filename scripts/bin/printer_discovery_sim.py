#!/usr/bin/env python3
"""
Scribe Printer Discovery Test Script

This script simulates multiple Scribe printers for testing the automatic discovery system
when you only have one physical device. It creates fake MQTT printer status messages
that your real Scribe will receive and display in its web interface.

Purpose:
- Simulate various network scenarios (printers going online/offline)
- Validate that the web interface correctly displays discovered printers
- Test MQTT Last Will & Testament (LWT) functionality

Installation & Setup:

Quick start:
source .venv/bin/activate
python3 test_printer_discovery.py --scenario home

This project already includes a virtual environment (.venv/). To use it:

    source .venv/bin/activate  # On Windows: .venv\\Scripts\\activate
    pip install -r requirements.txt

Or if you need to create a new virtual environment:
    python3 -m venv .venv
    source .venv/bin/activate
    pip install -r requirements.txt

Configuration:
Copy .mqtt_secrets.example to .mqtt_secrets and edit with your MQTT broker credentials:
    cp .mqtt_secrets.example .mqtt_secrets
    # Edit .mqtt_secrets with your actual MQTT settings

Usage Examples:

1. Quick office scenario (4 printers):
   python3 scripts/bin/printer_discovery_sim.py --scenario office

2. Home scenario (2 printers):
   python3 scripts/bin/printer_discovery_sim.py --scenario home

3. Interactive mode for manual testing:
   python3 scripts/bin/printer_discovery_sim.py
   > start Alice 1.2.0
   > start Bob 1.0.0
   > stop Alice
   > quit

4. Custom MQTT broker:
   python3 scripts/bin/printer_discovery_sim.py --host mqtt.example.com --username user --password pass

5. Test chaos mode (rapid connect/disconnect):
   python3 scripts/bin/printer_discovery_sim.py --scenario chaos

6. Test both shutdown types:
   python3 scripts/bin/printer_discovery_sim.py --scenario home
   # Then while running:
   # - Press 'x' + Enter = ABRUPT shutdown (triggers LWT)
   # - Press Ctrl+C = GRACEFUL shutdown (publishes offline status)

What to test after running:
- Check your Scribe's index.html page - fake printers should appear in the remote printer choices
- Visit /diagnostics.html to see discovery system status
- Monitor /mqtt-printers (SSE) endpoint for real-time printer updates
- Stop/start printers to see them disappear/appear in real-time

Interactive Commands:
- start <name> [firmware]  - Start a printer (e.g., 'start Alice 1.2.0')
- stop <name>             - Stop a printer (graceful shutdown: publishes offline status)
- stop <name> abrupt      - Stop a printer (abrupt: triggers LWT)
- status <name> <status>  - Update printer status (online/offline)
- list                    - List active printers
- scenario <name>         - Run predefined scenarios (office/home/mixed/chaos)
- scenario list           - List available scenarios with descriptions
- help                    - Show available scenarios
- quit                    - Exit (graceful shutdown of all printers)

NOTE: Ctrl+C performs GRACEFUL shutdown - publishes offline status before disconnecting.
      This simulates a proper power-down sequence, not a network failure.

Author: Generated for Scribe ESP32-C3 Thermal Printer project
Date: August 2025
"""

import json
import time
import random
import argparse
import threading
import sys
import select
import os
from datetime import datetime, timezone

# Try to import required libraries with helpful error messages
try:
    import paho.mqtt.client as mqtt
except ImportError as e:
    print("❌ Error: paho-mqtt library not found!")
    print("📦 Please install it using:")
    print("   pip install paho-mqtt")
    print("   or")
    print("   pip install --break-system-packages paho-mqtt")
    print("   or set up a virtual environment:")
    print(
        "   python3 -m venv venv && source venv/bin/activate && pip install paho-mqtt"
    )
    print(f"\nOriginal error: {e}")
    sys.exit(1)

try:
    from dotenv import load_dotenv  # type: ignore

    load_dotenv(".mqtt_secrets")  # Load .mqtt_secrets file if present
except ImportError:
    print(
        "⚠️  Warning: python-dotenv not found. Install with: pip install python-dotenv"
    )
    print("   Using command line arguments only.")
except (OSError, ValueError) as env_error:
    print(f"⚠️  Warning: Could not load .mqtt_secrets file: {env_error}")


def get_env_value(key: str, default: str = None) -> str:
    """Get environment variable with fallback to default"""
    value = os.getenv(key, default)
    if value is None:
        raise ValueError(f"Environment variable {key} not set and no default provided")
    return value


class KeyboardListener:
    def __init__(self, simulator):
        self.simulator = simulator
        self.running = True
        self.thread = None

    def start(self):
        """Start the keyboard listener in a background thread"""
        self.thread = threading.Thread(target=self._listen, daemon=True)
        self.thread.start()
        print("⌨️  Keyboard controls active:")
        print("   Press 'x' + Enter = ABRUPT shutdown (triggers LWT)")
        print("   Press Ctrl+C = GRACEFUL shutdown (publishes offline status)")
        print()

    def stop(self):
        """Stop the keyboard listener"""
        self.running = False

    def _listen(self):
        """Listen for keyboard input in background thread"""
        while self.running:
            try:
                if sys.stdin in select.select([sys.stdin], [], [], 0.1)[0]:
                    key = sys.stdin.readline().strip().lower()
                    if key == "x":
                        print(
                            "💥 ABRUPT shutdown triggered! (simulates power loss/network failure)"
                        )
                        self._abrupt_shutdown()
                        break
                    elif key == "help" or key == "h":
                        print("⌨️  Keyboard controls:")
                        print("   'x' + Enter = ABRUPT shutdown (triggers LWT)")
                        print(
                            "   Ctrl+C = GRACEFUL shutdown (publishes offline status)"
                        )
            except (OSError, IOError, KeyboardInterrupt):
                # Handle any errors silently (like Windows compatibility issues)
                time.sleep(0.1)

    def _abrupt_shutdown(self):
        """Perform abrupt shutdown of all printers"""
        print("🔥 Forcing abrupt disconnection for all printers...")
        for name in list(self.simulator.clients.keys()):
            print(f"⚡ {name}: Triggering LWT via socket closure")
            try:
                # Force socket closure to trigger LWT
                if hasattr(self.simulator.clients[name], "_sock"):
                    # pylint: disable-next=protected-access
                    self.simulator.clients[name]._sock.close()
            except (AttributeError, OSError):
                pass
            self.simulator.clients[name].loop_stop()
            del self.simulator.clients[name]

        print(
            "💀 All printers abruptly disconnected - LWT messages should be triggered"
        )
        print("🛑 Exiting script...")
        os._exit(0)  # pylint: disable=protected-access


class ScribePrinterSimulator:
    def __init__(
        self, broker_host, broker_port, username=None, password=None, use_tls=False
    ):
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.username = username
        self.password = password
        self.use_tls = use_tls
        self.clients = {}

    def create_printer_payload(
        self, name, ip_suffix, firmware="1.0.0", status="online"
    ):
        """Create a realistic printer status payload"""
        printer_id = f"scribe-{name.lower()}"
        # Format timestamp to match real printer: "2025-08-12T19:01:09Z"
        timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        return {
            "name": name,
            "firmware_version": firmware,
            "mdns": f"{printer_id}.local",
            "ip_address": f"192.168.1.{ip_suffix}",
            "status": status,
            "last_power_on": timestamp,
            "timezone": "Europe/London",
        }

    def create_offline_payload(self, name):
        """Create a simple offline payload (just name and status)"""
        return {"name": name, "status": "offline"}

    def setup_client(self, printer_name, _ip_suffix, _firmware="1.0.0"):
        """Setup MQTT client for a simulated printer"""
        client_id = f"ScribePrinter-{random.randint(1000, 9999)}"
        # Use the new paho-mqtt 2.x API with VERSION2 (modern approach)
        client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2, client_id=client_id
        )

        if self.username:
            client.username_pw_set(self.username, self.password)

        if self.use_tls:
            import ssl

            try:
                # Try multiple SSL configuration approaches
                context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
                context.check_hostname = False
                context.verify_mode = ssl.CERT_NONE
                client.tls_set_context(context)
            except (AttributeError, TypeError, OSError):
                # Fallback for older paho-mqtt versions or SSL issues
                try:
                    client.tls_set(
                        ca_certs=None,
                        certfile=None,
                        keyfile=None,
                        cert_reqs=ssl.CERT_NONE,
                        tls_version=ssl.PROTOCOL_TLS,
                        ciphers=None,
                    )
                except (AttributeError, TypeError, OSError):
                    # Last resort - basic TLS
                    client.tls_set()

        # Setup LWT (Last Will & Testament) with simple offline payload
        topic = f"scribe/printer-status/{printer_name.lower()}"
        lwt_payload = self.create_offline_payload(printer_name)
        client.will_set(topic, json.dumps(lwt_payload), qos=1, retain=True)

        def on_connect(_client, _userdata, _flags, reason_code, _properties):
            if reason_code == 0:
                print(f"✅ {printer_name}: Connected to MQTT broker")
            else:
                print(f"❌ {printer_name}: Failed to connect (code {reason_code})")

        def on_disconnect(_client, _userdata, _flags, reason_code, _properties):
            if reason_code != 0:
                print(f"🔌 {printer_name}: Unexpectedly disconnected (LWT triggered)")
            else:
                print(f"🔌 {printer_name}: Clean disconnect")

        client.on_connect = on_connect
        client.on_disconnect = on_disconnect

        return client

    def start_printer(self, name, ip_suffix, firmware="1.0.0"):
        """Start simulating a printer"""
        print(f"🖨️  Starting printer: {name}")

        client = self.setup_client(name, ip_suffix, firmware)

        try:
            print(
                f"🔗 {name}: Attempting connection to {self.broker_host}:{self.broker_port}"
            )
            client.connect(self.broker_host, self.broker_port, 60)
            client.loop_start()

            # Wait a moment for connection
            time.sleep(1)

            # Publish initial status
            topic = f"scribe/printer-status/{name.lower()}"
            payload = self.create_printer_payload(name, ip_suffix, firmware, "online")

            result = client.publish(topic, json.dumps(payload), qos=1, retain=True)
            if result.rc == 0:
                print(f"📡 {name}: Published status to {topic}")
                print(f"   IP: 192.168.1.{ip_suffix}, Firmware: {firmware}")
            else:
                print(f"❌ {name}: Failed to publish status")

            self.clients[name] = client
            return True

        except (OSError, ValueError, ConnectionRefusedError) as e:
            error_msg = str(e)
            print(f"❌ {name}: Connection failed - {e}")

            if (
                "SSL" in error_msg or "TLS" in error_msg or "EOF occurred" in error_msg
            ) and self.use_tls:
                print("💡 SSL/TLS connection issue detected")
                print("💡 Try: --tls false for non-SSL connection")
                print("💡 Or check if broker requires different SSL settings")
                print(f"🔄 {name}: Attempting fallback to non-TLS connection...")
                return self._try_fallback_connection(name, ip_suffix, firmware)
            elif "Connection refused" in error_msg:
                print(f"💡 Current: {self.broker_host}:{self.broker_port}")
                print("💡 Check host/port settings")
            else:
                print("💡 Check network connectivity and broker settings")
            return False

    def _try_fallback_connection(self, name, ip_suffix, firmware="1.0.0"):
        """Try connecting without TLS as fallback"""
        print(f"🔄 {name}: Trying fallback connection without TLS...")

        # Create a new client without TLS
        fallback_sim = ScribePrinterSimulator(
            self.broker_host,
            1883,  # Standard MQTT port without TLS
            self.username,
            self.password,
            False,  # Disable TLS
        )

        try:
            client = fallback_sim.setup_client(name, ip_suffix, firmware)
            client.connect(fallback_sim.broker_host, fallback_sim.broker_port, 60)
            client.loop_start()

            time.sleep(1)

            # Publish initial status
            topic = f"scribe/printer-status/{name.lower()}"
            payload = self.create_printer_payload(name, ip_suffix, firmware, "online")

            result = client.publish(topic, json.dumps(payload), qos=1, retain=True)
            if result.rc == 0:
                print(f"✅ {name}: Fallback connection successful (non-TLS)")
                print(f"📡 {name}: Published status to {topic}")
                print(f"   IP: 192.168.1.{ip_suffix}, Firmware: {firmware}")
                self.clients[name] = client
                return True
            else:
                print(f"❌ {name}: Fallback connection failed to publish")
                return False

        except (OSError, ValueError, ConnectionRefusedError) as fallback_error:
            print(f"❌ {name}: Fallback connection also failed - {fallback_error}")
            return False

    def stop_printer(self, name, graceful=True):
        """Stop simulating a printer with graceful or abrupt disconnection"""
        if name in self.clients:
            print(f"🛑 Stopping printer: {name}")

            if graceful:
                # Graceful shutdown: publish offline status first, then clean disconnect
                print(f"👋 {name}: Publishing graceful offline status")
                topic = f"scribe/printer-status/{name.lower()}"

                # Create simple offline payload using helper method
                offline_payload = self.create_offline_payload(name)

                # Publish offline status
                result = self.clients[name].publish(
                    topic, json.dumps(offline_payload), qos=1, retain=True
                )
                if result.rc == 0:
                    print(f"📡 {name}: Published graceful offline status")
                else:
                    print(f"❌ {name}: Failed to publish offline status")

                # Wait a moment for message to be sent
                time.sleep(0.5)

                # Clean disconnect (won't trigger LWT)
                self.clients[name].disconnect()
                self.clients[name].loop_stop()
                print(f"🔌 {name}: Graceful disconnect completed")
            else:
                # Abrupt disconnection to trigger LWT
                print(f"💀 {name}: Forcing abrupt disconnection to trigger LWT")
                try:
                    # Force socket closure to trigger LWT
                    if hasattr(self.clients[name], "_sock"):
                        # pylint: disable-next=protected-access
                        self.clients[name]._sock.close()
                except (AttributeError, OSError):
                    pass
                self.clients[name].loop_stop()
                print(f"⚡ {name}: LWT should be triggered")

            del self.clients[name]
        else:
            print(f"❌ Printer {name} not found")

    def update_printer_status(self, name, status="online"):
        """Update a printer's status"""
        if name not in self.clients:
            print(f"❌ Printer {name} not found")
            return

        topic = f"scribe/printer-status/{name.lower()}"

        # Get current payload and update status
        ip_suffix = 100 + len(self.clients)  # Simple IP assignment
        payload = self.create_printer_payload(name, ip_suffix, "1.0.0", status)

        result = self.clients[name].publish(
            topic, json.dumps(payload), qos=1, retain=True
        )
        if result.rc == 0:
            print(f"📡 {name}: Updated status to '{status}'")
        else:
            print(f"❌ {name}: Failed to update status")

    def list_active_printers(self):
        """List currently active simulated printers"""
        if not self.clients:
            print("No active printers")
        else:
            print("Active printers:")
            for name in self.clients.keys():
                print(f"  - {name}")

    def stop_all(self):
        """Stop all simulated printers"""
        print("🛑 Stopping all printers...")
        for name in list(self.clients.keys()):
            self.stop_printer(name)


def run_interactive_mode(simulator):
    """Run interactive testing mode"""
    print("\n=== Scribe Printer Discovery Test ===")
    print("Commands:")
    print("  start <name> [firmware] - Start a printer (e.g., 'start Alice 1.2.0')")
    print(
        "  stop <name>            - Stop a printer (graceful: publishes offline status)"
    )
    print("  stop <name> abrupt     - Stop a printer (abrupt: triggers LWT)")
    print("  status <name> <status> - Update printer status (online/offline)")
    print("  list                   - List active printers")
    print("  scenario <name>        - Run a test scenario")
    print("  scenario list          - List available scenarios")
    print("  help                   - Show this help")
    print("  quit                   - Exit (graceful shutdown)")
    print("\nAvailable scenarios: office, home, mixed, chaos")
    print("💡 TIP: Ctrl+C = GRACEFUL shutdown (publishes offline status)")
    print("📱 Now check your Scribe's web interface to see discovered printers!\n")

    while True:
        try:
            cmd = input("scribe-test> ").strip().split()

            if not cmd:
                continue

            if cmd[0] == "quit" or cmd[0] == "exit":
                break
            elif cmd[0] == "start" and len(cmd) >= 2:
                firmware = cmd[2] if len(cmd) > 2 else "1.0.0"
                ip_suffix = 100 + len(simulator.clients) + 1
                simulator.start_printer(cmd[1], ip_suffix, firmware)
            elif cmd[0] == "stop" and len(cmd) >= 2:
                # Check if user wants abrupt disconnection (stop <name> abrupt)
                graceful = True
                if len(cmd) >= 3 and cmd[2].lower() in ["abrupt", "lwt", "force"]:
                    graceful = False
                simulator.stop_printer(cmd[1], graceful)
            elif cmd[0] == "status" and len(cmd) >= 3:
                simulator.update_printer_status(cmd[1], cmd[2])
            elif cmd[0] == "list":
                simulator.list_active_printers()
            elif cmd[0] == "scenario" and len(cmd) >= 2:
                if cmd[1] == "list":
                    list_scenarios()
                else:
                    run_scenario(simulator, cmd[1])
            elif cmd[0] == "help":
                print("Available scenarios: office, home, mixed, chaos")
                print(
                    "Example: 'scenario office' or 'scenario list' or 'start TestPrinter 1.1.0'"
                )
            else:
                print("Unknown command. Type 'help' for available commands.")

        except KeyboardInterrupt:
            break
        except (ValueError, OSError) as e:
            print(f"Error: {e}")

    simulator.stop_all()


def list_scenarios():
    """List all available scenarios with descriptions"""
    scenarios = {
        "office": "Multiple printers with different firmware versions (Reception, Alice, Bob, DevTeam)",
        "home": "Small home setup with 2 printers (Kitchen, Study)",
        "mixed": "Simulates network issues and reconnections (some stable, some unstable)",
        "chaos": "Rapid connect/disconnect to test system resilience (5 printers cycling)",
    }

    print("\n📋 Available scenarios:")
    for name, description in scenarios.items():
        print(f"  {name:8} - {description}")
    print("\nUsage: scenario <name>")
    print("Example: scenario office\n")


def run_scenario(simulator, scenario_name):
    """Run predefined test scenarios"""
    print(f"🎬 Running scenario: {scenario_name}")

    if scenario_name == "office":
        # Office scenario - multiple printers with different versions
        print("📋 Office scenario: Multiple printers with different firmware versions")
        printers = [
            ("Reception", "1.0.0"),
            ("Alice", "1.1.0"),
            ("Bob", "1.0.0"),
            ("DevTeam", "1.2.0-beta"),
        ]
        for i, (name, firmware) in enumerate(printers):
            simulator.start_printer(name, 101 + i, firmware)
            time.sleep(0.5)

    elif scenario_name == "home":
        # Home scenario - fewer printers
        print("🏠 Home scenario: Small home setup")
        printers = [("Kitchen", "1.0.0"), ("Study", "1.1.0")]
        for i, (name, firmware) in enumerate(printers):
            simulator.start_printer(name, 101 + i, firmware)
            time.sleep(0.5)

    elif scenario_name == "mixed":
        # Mixed scenario - start some, then simulate disconnections
        print("🔄 Mixed scenario: Simulating network issues and reconnections")
        simulator.start_printer("Stable", 101, "1.0.0")
        time.sleep(1)
        simulator.start_printer("Unstable", 102, "1.1.0")
        time.sleep(2)
        print("💥 Simulating network issue...")
        simulator.stop_printer("Unstable")
        time.sleep(1)
        simulator.start_printer("AnotherStable", 103, "1.2.0")

    elif scenario_name == "chaos":
        # Chaos scenario - rapid connect/disconnect
        print("🌪️  Chaos mode: Rapid connect/disconnect to test resilience")
        for i in range(5):
            name = f"Chaos{i}"
            simulator.start_printer(name, 120 + i, f"1.{i}.0")
            time.sleep(0.2)

        time.sleep(2)
        print("🔥 Starting chaos disconnections...")

        for i in range(5):
            name = f"Chaos{i}"
            simulator.stop_printer(name)
            time.sleep(0.3)
    else:
        print(f"Unknown scenario: {scenario_name}")
        print("Available scenarios: office, home, mixed, chaos")


def main():
    parser = argparse.ArgumentParser(description="Scribe Printer Discovery Test Script")

    # Get defaults from environment variables
    default_host = get_env_value("MQTT_HOST", "localhost")
    default_port = int(get_env_value("MQTT_PORT", "1883"))
    default_username = get_env_value("MQTT_USERNAME", "")
    default_password = get_env_value("MQTT_PASSWORD", "")
    default_tls = get_env_value("MQTT_USE_TLS", "false").lower() == "true"

    parser.add_argument(
        "--host",
        default=default_host,
        help=f"MQTT broker host (default: {default_host})",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=default_port,
        help=f"MQTT broker port (default: {default_port})",
    )
    parser.add_argument("--username", default=default_username, help="MQTT username")
    parser.add_argument("--password", default=default_password, help="MQTT password")

    # TLS arguments - allow both enabling and disabling
    tls_group = parser.add_mutually_exclusive_group()
    tls_group.add_argument("--tls", action="store_true", help="Enable TLS/SSL")
    tls_group.add_argument(
        "--no-tls", action="store_true", help="Disable TLS/SSL (use plain MQTT)"
    )

    parser.add_argument(
        "--scenario", help="Run a specific scenario (office, home, mixed, chaos)"
    )

    args = parser.parse_args()

    # Determine TLS setting with priority: command line > environment > default
    if args.no_tls:
        use_tls = False
        # Auto-adjust port if using default TLS port but disabling TLS
        if args.port == default_port and default_port == 8883:
            broker_port = 1883  # Standard non-TLS MQTT port
            print("💡 Auto-adjusting port to 1883 for non-TLS connection")
        else:
            broker_port = args.port
    elif args.tls:
        use_tls = True
        broker_port = args.port
    else:
        use_tls = default_tls
        broker_port = args.port

    print("🚀 Scribe Printer Discovery Test Script")
    print(f"📡 Connecting to: {args.host}:{broker_port}")
    print(f"🔐 TLS: {'Enabled' if use_tls else 'Disabled'}")
    if use_tls:
        print("💡 If you experience SSL/TLS connection issues, try: --no-tls")
    print()

    # Create simulator
    simulator = ScribePrinterSimulator(
        args.host, broker_port, args.username, args.password, use_tls
    )

    try:
        if args.scenario:
            # Run specific scenario and keep running
            run_scenario(simulator, args.scenario)
            print(f"\n✅ Scenario '{args.scenario}' completed.")
            print(
                "💡 Now check your Scribe's web interface to see the discovered printers!"
            )
            print("📱 Visit: http://scribe.local or your Scribe's IP address")
            print(
                "🔍 Check: index.html (printer dropdown), diagnostics.html, /events (SSE)"
            )
            print()

            # Start keyboard listener for abrupt shutdown testing
            listener = KeyboardListener(simulator)
            listener.start()

            # Keep running until interrupted
            while True:
                time.sleep(1)
        else:
            # Run interactive mode
            run_interactive_mode(simulator)

    except KeyboardInterrupt:
        print("\n🛑 Ctrl+C detected - performing GRACEFUL shutdown...")
        print("📡 Publishing offline status for all printers before disconnecting...")
        simulator.stop_all()
        print("👋 All simulated printers gracefully stopped. Goodbye!")
    except (OSError, ValueError, RuntimeError) as e:
        print(f"💥 Error: {e}")
        simulator.stop_all()


if __name__ == "__main__":
    main()
