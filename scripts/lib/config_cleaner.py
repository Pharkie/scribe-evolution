#!/usr/bin/env python3
"""
Shared configuration cleaning utilities.
Contains the single source of truth for secret detection and replacement patterns.
"""

import re
import sys


def get_secret_patterns():
    """
    Return the secret detection and replacement patterns.
    This is the single source of truth for what constitutes a secret.
    """
    return [
        # WiFi credentials - detect quoted strings (including multi-line)
        (
            r'(defaultWifiSSID\s*=\s*)"[^"]*"(?:\s*"[^"]*")*',
            r'\1"YOUR_WIFI_SSID"',
        ),
        (
            r'(defaultWifiPassword\s*=\s*)"[^"]*"(?:\s*"[^"]*")*',
            r'\1"YOUR_WIFI_PASSWORD"',
        ),
        # MQTT broker - detect cloud provider domains (including multi-line)
        (
            r'(defaultMqttServer\s*=\s*)"[^"]*\.'
            r'(?:hivemq\.cloud|amazonaws\.com|azure\.com)"(?:\s*"[^"]*")*',
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
        # Device owner - detect non-placeholder names (multi-line)
        (
            r'(defaultDeviceOwner\s*=\s*)"(?!YOUR_)[^"]*"'
            r'(?:\s*"[^"]*")*',
            r'\1"YOUR_DEVICE_NAME"',
        ),
        # API tokens - detect base64-like or sk- prefixes (multi-line)
        (
            r'(defaultChatgptApiToken\s*=\s*)"(?:sk-[^"]*|'
            r'[A-Za-z0-9+/]{20,})"(?:\s*"[^"]*")*',
            r'\1"YOUR_OPENAI_API_KEY"',
        ),
        # BetterStack - detect alphanumeric tokens and URLs (multi-line)
        (
            r'(betterStackToken\s*=\s*)"[A-Za-z0-9]{15,}"'
            r'(?:\s*"[^"]*")*',
            r'\1"YOUR_BETTERSTACK_TOKEN"',
        ),
        (
            r'(betterStackEndpoint\s*=\s*)"https://[^"]*'
            r'betterstackdata\.com[^"]*"(?:\s*"[^"]*")*',
            r'\1"YOUR_BETTERSTACK_ENDPOINT"',
        ),
    ]


def get_suspicious_patterns():
    """
    Return patterns that might indicate secrets not caught by main patterns.
    Used for validation to ensure no secrets slip through.
    """
    return [
        # Long alphanumeric strings that might be tokens/keys
        (
            r'"[A-Za-z0-9+/]{32,}"',
            "Long alphanumeric string (possible token/key)",
        ),
        # API key patterns
        (r'"sk-[A-Za-z0-9\-_]{20,}"', "OpenAI-style API key"),
        (
            r'"[A-Za-z0-9]{40,}"',
            "Long alphanumeric string (possible API key)",
        ),
        # URLs with credentials
        (
            r'"https://[^@"]*:[^@"]*@[^"]*"',
            "URL with embedded credentials",
        ),
        # JWT-like tokens (three base64 segments)
        (
            r'"[A-Za-z0-9\-_]{20,}\.[A-Za-z0-9\-_]{20,}\.'
            r'[A-Za-z0-9\-_]{20,}"',
            "JWT-like token",
        ),
        # Base64-encoded data
        (r'"[A-Za-z0-9+/]{64,}={0,2}"', "Base64-encoded data"),
    ]


def get_placeholder_keywords():
    """
    Return keywords indicating a value is a placeholder, not a secret.
    """
    return [
        "YOUR_",
        "EXAMPLE_",
        "PLACEHOLDER_",
        "TEST_",
        "DEMO_",
    ]


def clean_secrets_from_content(content):
    """
    Remove secrets from configuration content using standard patterns.

    Args:
        content (str): Configuration file content

    Returns:
        str: Content with secrets replaced by placeholders

    Raises:
        SystemExit: If potential secrets are detected after cleaning
    """
    patterns = get_secret_patterns()

    # Apply pattern replacements with DOTALL for multi-line strings
    for pattern, replacement in patterns:
        content = re.sub(pattern, replacement, content, flags=re.DOTALL)

    # Validate that no secrets remain
    validate_no_secrets_remain(content)

    return content


def validate_no_secrets_remain(content):
    """
    Check if any potential secrets remain in the cleaned content.

    Args:
        content (str): Cleaned configuration content

    Raises:
        SystemExit: If potential secrets are found
    """
    suspicious_patterns = get_suspicious_patterns()
    placeholder_keywords = get_placeholder_keywords()
    potential_secrets = []

    for pattern, description in suspicious_patterns:
        matches = re.findall(pattern, content)
        for match in matches:
            # Skip known placeholder values
            is_placeholder = any(
                placeholder in match for placeholder in placeholder_keywords
            )
            if not is_placeholder:
                potential_secrets.append(f"{description}: {match}")

    # If potential secrets found, report them and exit with error
    if potential_secrets:
        print("⚠️  POTENTIAL SECRETS DETECTED in cleaned content:")
        for secret in potential_secrets:
            print(f"   - {secret}")
        print(
            "\n❌ Cleaning aborted. "
            "Please review the patterns in config_cleaner.py"
        )
        print(
            "   Add specific patterns for any legitimate secrets "
            "found above."
        )
        sys.exit(1)  # Exit with error code to stop the build


def add_example_file_metadata(content):
    """
    Update file metadata and add instructions for .example files.

    Args:
        content (str): Configuration content

    Returns:
        str: Content with example file metadata and instructions
    """
    # Update file metadata
    content = content.replace(
        "@file device_config.h", "@file device_config.h.example"
    )
    content = content.replace(
        "@brief Configuration settings for",
        "@brief Configuration settings template for",
    )

    # Add instructions after the license section
    instructions = (
        " * INSTRUCTIONS: Copy this file to device_config.h and fill in "
        "your actual credentials.\n"
        " * The device_config.h file is gitignored to keep your secrets safe."
    )

    # Find the license section ending and add instructions
    # Remove any trailing empty comment lines before the closing */
    if " */\n" in content:
        parts = content.split(" */\n", 1)
        if len(parts) == 2:
            # Remove trailing empty comment lines from the first part
            first_part = parts[0].rstrip()
            if first_part.endswith(" *"):
                first_part = first_part[:-2].rstrip()
            content = (
                first_part + "\n *\n" + instructions + "\n */\n" + parts[1]
            )

    return content

