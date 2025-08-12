#!/usr/bin/env python3
"""
Scribe Printer Discovery Test Script

This script simulates multiple Scribe printers for testing the automatic discovery system
when you only have one physical device. It creates fake MQTT printer status messages
that your real Scribe will receive and display in its web interface.

Purpose:
- Test the printer discovery feature without needing multiple physical Scribe printers
- Simulate various network scenarios (printers going online/offline)
- Validate that the web interface correctly displays discovered printers
- Test MQTT Last Will & Testament (LWT) functionality

Installation & Setup:
This project already includes a virtual environment (.venv/). To use it:

    source .venv/bin/activate  # On Windows: .venv\\Scripts\\activate
    pip install -r requirements.txt

Or if you need to create a new virtual environment:
    python3 -m venv .venv
    source .venv/bin/activate
    pip install -r requirements.txt

Usage Examples:

1. Quick office scenario (4 printers):
   python test_printer_discovery.py --scenario office

2. Home scenario (2 printers):
   python test_printer_discovery.py --scenario home

3. Interactive mode for manual testing:
   python test_printer_discovery.py
   > start Alice 1.2.0
   > start Bob 1.0.0
   > stop Alice
   > quit

4. Custom MQTT broker:
   python test_printer_discovery.py --host mqtt.example.com --username user --password pass

5. Test chaos mode (rapid connect/disconnect):
   python test_printer_discovery.py --scenario chaos

What to test after running:
- Check your Scribe's index.html page - fake printers should appear in the remote printer choices
- Visit /diagnostics.html to see discovery system status
- Try /api/discovered-printers endpoint to see raw discovery data
- Stop/start printers to see them disappear/appear in real-time

Interactive Commands:
- start <name> [firmware]  - Start a printer (e.g., 'start Alice 1.2.0')
- stop <name>             - Stop a printer (triggers LWT)
- status <name> <status>  - Update printer status (online/offline)
- list                    - List active printers
- scenario <name>         - Run predefined scenarios (office/home/mixed/chaos)
- help                    - Show available scenarios
- quit                    - Exit and trigger LWT for all printers

Author: Generated for Scribe ESP32-C3 Thermal Printer project
Date: August 2025
"""

import json
import time
import random
import argparse
from datetime import datetime, timezone

# Try to import paho.mqtt.client with helpful error message
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
    exit(1)


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

    def setup_client(self, printer_name, ip_suffix, firmware="1.0.0"):
        """Setup MQTT client for a simulated printer"""
        client_id = f"ScribePrinter-{random.randint(1000, 9999)}"
        # Use the new paho-mqtt 2.x API
        client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION1, client_id=client_id
        )

        if self.username:
            client.username_pw_set(self.username, self.password)

        if self.use_tls:
            client.tls_set()

        # Setup LWT (Last Will & Testament) with complete payload
        topic = f"scribe/printer-status/{printer_name.lower()}"
        lwt_payload = self.create_printer_payload(printer_name, ip_suffix, firmware, "offline")
        client.will_set(topic, json.dumps(lwt_payload), qos=1, retain=True)

        def on_connect(client, userdata, flags, rc):
            if rc == 0:
                print(f"✅ {printer_name}: Connected to MQTT broker")
            else:
                print(f"❌ {printer_name}: Failed to connect (code {rc})")

        def on_disconnect(client, userdata, rc):
            if rc != 0:
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

        except Exception as e:
            print(f"❌ {name}: Error starting printer - {e}")
            return False

    def stop_printer(self, name, graceful=True):
        """Stop simulating a printer with graceful or abrupt disconnection"""
        if name in self.clients:
            print(f"🛑 Stopping printer: {name}")
            
            if graceful:
                # Graceful shutdown: publish offline status first, then clean disconnect
                print(f"👋 {name}: Publishing graceful offline status")
                topic = f"scribe/printer-status/{name.lower()}"
                
                # Get printer info for complete offline payload
                ip_suffix = 100 + list(self.clients.keys()).index(name) + 1
                offline_payload = self.create_printer_payload(name, ip_suffix, "1.0.0", "offline")
                
                # Publish offline status
                result = self.clients[name].publish(topic, json.dumps(offline_payload), qos=1, retain=True)
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
                    self.clients[name]._sock.close()
                except:
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
    print("  stop <name>            - Stop a printer (triggers LWT)")
    print("  status <name> <status> - Update printer status (online/offline)")
    print("  list                   - List active printers")
    print("  scenario <name>        - Run a test scenario")
    print("  help                   - Show this help")
    print("  quit                   - Exit")
    print("\nAvailable scenarios: office, home, mixed, chaos")
    print("Now check your Scribe's web interface to see discovered printers!\n")

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
                simulator.stop_printer(cmd[1])
            elif cmd[0] == "status" and len(cmd) >= 3:
                simulator.update_printer_status(cmd[1], cmd[2])
            elif cmd[0] == "list":
                simulator.list_active_printers()
            elif cmd[0] == "scenario" and len(cmd) >= 2:
                run_scenario(simulator, cmd[1])
            elif cmd[0] == "help":
                print("Available scenarios: office, home, mixed, chaos")
                print("Example: 'scenario office' or 'start TestPrinter 1.1.0'")
            else:
                print("Unknown command. Type 'help' for available commands.")

        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"Error: {e}")

    simulator.stop_all()


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
        simulator.start_printer("NewPrinter", 103, "1.2.0")

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
    parser.add_argument(
        "--host",
        default="a0829e28cf7842e9ba6f1e9830cdab3c.s1.eu.hivemq.cloud",
        help="MQTT broker host (default: HiveMQ Cloud)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8883,
        help="MQTT broker port (default: 8883 for TLS)",
    )
    parser.add_argument("--username", default="adammain", help="MQTT username")
    parser.add_argument(
        "--password", default="wqk*uwv5KMX4cwd7pxy", help="MQTT password"
    )
    parser.add_argument(
        "--tls", action="store_true", default=True, help="Use TLS/SSL (default: True)"
    )
    parser.add_argument(
        "--scenario", help="Run a specific scenario (office, home, mixed, chaos)"
    )

    args = parser.parse_args()

    print("🚀 Scribe Printer Discovery Test Script")
    print(f"📡 Connecting to: {args.host}:{args.port}")
    print(f"🔐 TLS: {'Enabled' if args.tls else 'Disabled'}")
    print()

    # Create simulator
    simulator = ScribePrinterSimulator(
        args.host, args.port, args.username, args.password, args.tls
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
                "🔍 Check: index.html (printer dropdown), diagnostics.html, /api/discovered-printers"
            )
            print("⏹️  Press Ctrl+C to stop all simulated printers and exit.")

            # Keep running until interrupted
            while True:
                time.sleep(1)
        else:
            # Run interactive mode
            run_interactive_mode(simulator)

    except KeyboardInterrupt:
        print("\n🛑 Shutting down...")
        simulator.stop_all()
        print("👋 All simulated printers stopped. Goodbye!")
    except Exception as e:
        print(f"💥 Error: {e}")
        simulator.stop_all()


if __name__ == "__main__":
    main()
