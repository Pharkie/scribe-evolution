#!/usr/bin/env python3
"""
Generate config.h.example from config.h by replacing secrets with placeholders.
This script uses pattern detection instead of hardcoded secrets for security.
"""

import re
import os
import sys

# This import is required for PlatformIO extra scripts
# Import is only available when run by PlatformIO, not in regular Python context
Import("env")  # type: ignore # pylint: disable=undefined-variable


def generate_example_config():
    config_path = "src/core/config.h"
    example_path = "src/core/config.h.example"

    if not os.path.exists(config_path):
        print(f"Error: {config_path} not found")
        sys.exit(1)

    # Read the main config
    with open(config_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Pattern-based replacements (no actual secrets stored here!)
    # Updated patterns to handle multi-line string literals
    patterns = [
        # WiFi credentials - detect quoted strings after SSID/Password (including multi-line)
        (r'(defaultWifiSSID\s*=\s*)"[^"]*"(?:\s*"[^"]*")*', r'\1"YOUR_WIFI_SSID"'),
        (
            r'(defaultWifiPassword\s*=\s*)"[^"]*"(?:\s*"[^"]*")*',
            r'\1"YOUR_WIFI_PASSWORD"',
        ),
        # MQTT broker - detect domains ending in common cloud providers (including multi-line)
        (
            r'(defaultMqttServer\s*=\s*)"[^"]*\.(?:hivemq\.cloud|amazonaws\.com|azure\.com)"(?:\s*"[^"]*")*',
            r'\1"YOUR_MQTT_SERVER.hivemq.cloud"',
        ),
        (
            r'(defaultMqttUsername\s*=\s*)"[^"]*"(?:\s*"[^"]*")*',
            r'\1"YOUR_MQTT_USERNAME"',
        ),
        (
            r'(defaultMqttPassword\s*=\s*)"[^"]*"(?:\s*"[^"]*")*',
            r'\1"YOUR_MQTT_PASSWORD"',
        ),
        # Device owner - detect non-placeholder names (including multi-line)
        (
            r'(defaultDeviceOwner\s*=\s*)"(?!YOUR_)[^"]*"(?:\s*"[^"]*")*',
            r'\1"YOUR_DEVICE_NAME"',
        ),
        # API tokens - detect base64-like patterns or sk- prefixes (including multi-line)
        (
            r'(defaultChatgptApiToken\s*=\s*)"(?:sk-[^"]*|[A-Za-z0-9+/]{20,})"(?:\s*"[^"]*")*',
            r'\1"YOUR_OPENAI_API_KEY"',
        ),
        # BetterStack - detect alphanumeric tokens and URLs (including multi-line)
        (
            r'(betterStackToken\s*=\s*)"[A-Za-z0-9]{15,}"(?:\s*"[^"]*")*',
            r'\1"YOUR_BETTERSTACK_TOKEN"',
        ),
        (
            r'(betterStackEndpoint\s*=\s*)"https://[^"]*betterstackdata\.com[^"]*"(?:\s*"[^"]*")*',
            r'\1"YOUR_BETTERSTACK_ENDPOINT"',
        ),
    ]

    # Apply pattern replacements with DOTALL flag to handle multi-line strings
    for pattern, replacement in patterns:
        content = re.sub(pattern, replacement, content, flags=re.DOTALL)

    # General secret detection - catch anything that looks like a secret that wasn't replaced
    potential_secrets = []

    # Look for suspicious patterns that might be secrets
    suspicious_patterns = [
        # Long alphanumeric strings that might be tokens/keys
        (r'"[A-Za-z0-9+/]{32,}"', "Long alphanumeric string (possible token/key)"),
        # API key patterns
        (r'"sk-[A-Za-z0-9\-_]{20,}"', "OpenAI-style API key"),
        (r'"[A-Za-z0-9]{40,}"', "Long alphanumeric string (possible API key)"),
        # URLs with credentials
        (r'"https://[^@"]*:[^@"]*@[^"]*"', "URL with embedded credentials"),
        # JWT-like tokens (three base64 segments, not simple hostnames)
        (
            r'"[A-Za-z0-9\-_]{20,}\.[A-Za-z0-9\-_]{20,}\.[A-Za-z0-9\-_]{20,}"',
            "JWT-like token",
        ),
        # Base64-encoded data
        (r'"[A-Za-z0-9+/]{64,}={0,2}"', "Base64-encoded data"),
    ]

    for pattern, description in suspicious_patterns:
        matches = re.findall(pattern, content)
        for match in matches:
            # Skip known placeholder values
            if not any(
                placeholder in match
                for placeholder in [
                    "YOUR_",
                    "EXAMPLE_",
                    "PLACEHOLDER_",
                    "TEST_",
                    "DEMO_",
                ]
            ):
                potential_secrets.append(f"{description}: {match}")

    # If potential secrets found, report them and exit with error
    if potential_secrets:
        print("⚠️  POTENTIAL SECRETS DETECTED in generated config.h.example:")
        for secret in potential_secrets:
            print(f"   - {secret}")
        print(
            "\n❌ Generation aborted. Please review the patterns in generate_config_example.py"
        )
        print("   Add specific patterns for any legitimate secrets found above.")
        sys.exit(1)  # Exit with error code to stop the build process

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
