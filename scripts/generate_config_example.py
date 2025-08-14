#!/usr/bin/env python3
"""
Generate config.h.example from config.h by replacing secrets with placeholders.
This script uses pattern detection instead of hardcoded secrets for security.
"""

import re
import os

# This import is required for PlatformIO extra scripts
# Import is only available when run by PlatformIO, not in regular Python context
Import("env")  # type: ignore # pylint: disable=undefined-variable


def generate_example_config():
    config_path = "src/core/config.h"
    example_path = "src/core/config.h.example"

    if not os.path.exists(config_path):
        print(f"Error: {config_path} not found")
        return False

    # Read the main config
    with open(config_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Pattern-based replacements (no actual secrets stored here!)
    patterns = [
        # WiFi credentials - detect quoted strings after SSID/Password
        (r'(defaultWifiSSID\s*=\s*)"[^"]*"', r'\1"YOUR_WIFI_SSID"'),
        (r'(defaultWifiPassword\s*=\s*)"[^"]*"', r'\1"YOUR_WIFI_PASSWORD"'),
        # MQTT broker - detect domains ending in common cloud providers
        (
            r'(defaultMqttServer\s*=\s*)"[^"]*\.(?:hivemq\.cloud|amazonaws\.com|azure\.com)"',
            r'\1"YOUR_MQTT_SERVER.hivemq.cloud"',
        ),
        (r'(defaultMqttUsername\s*=\s*)"[^"]*"', r'\1"YOUR_MQTT_USERNAME"'),
        (r'(defaultMqttPassword\s*=\s*)"[^"]*"', r'\1"YOUR_MQTT_PASSWORD"'),
        # Device owner - detect non-placeholder names
        (r'(defaultDeviceOwner\s*=\s*)"(?!YOUR_)[^"]*"', r'\1"YOUR_DEVICE_NAME"'),
        # API tokens - detect base64-like patterns or sk- prefixes
        (
            r'(defaultChatgptApiToken\s*=\s*)"(?:sk-[^"]*|[A-Za-z0-9+/]{20,})"',
            r'\1"YOUR_OPENAI_API_KEY"',
        ),
        # BetterStack - detect alphanumeric tokens and URLs
        (r'(betterStackToken\s*=\s*)"[A-Za-z0-9]{15,}"', r'\1"YOUR_BETTERSTACK_TOKEN"'),
        (
            r'(betterStackEndpoint\s*=\s*)"https://[^"]*betterstackdata\.com[^"]*"',
            r'\1"YOUR_BETTERSTACK_ENDPOINT"',
        ),
    ]

    # Apply pattern replacements
    for pattern, replacement in patterns:
        content = re.sub(pattern, replacement, content)

    # Update file metadata
    content = content.replace("@file config.h", "@file config.h.example")
    content = content.replace(
        "@brief Configuration settings for",
        "@brief Configuration settings template for",
    )

    # Add instructions after the license section
    instructions = """
 * 
 * INSTRUCTIONS: Copy this file to config.h and fill in your actual credentials.
 * The config.h file is gitignored to keep your secrets safe."""

    content = content.replace(
        " * Based on the original Project Scribe by UrbanCircles.\n */",
        " * Based on the original Project Scribe by UrbanCircles."
        + instructions
        + "\n */",
    )

    # Write the example file
    with open(example_path, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"🍄 Generated {example_path} from {config_path}")
    return True


# Execute the generation when this script is loaded by PlatformIO
if __name__ == "__main__":
    generate_example_config()
else:
    # This runs when PlatformIO imports this script
    print("🔧 Running pre-build script: generating config.h.example...")
    generate_example_config()
