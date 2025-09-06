#!/usr/bin/env python3
"""
Generate config.h.example from config.h by replacing secrets with placeholders.
This script uses the shared config_cleaner module for consistent secret handling.
"""

import os
import sys
from pathlib import Path

# Ensure scripts/ is on sys.path and import shared cleaner from scripts/lib
# Use absolute path since __file__ may not be available in PlatformIO context
script_dir = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else 'scripts/pio'
scripts_root = os.path.join(script_dir, '..')
sys.path.insert(0, scripts_root)
from lib.config_cleaner import clean_secrets_from_content, add_example_file_metadata

# This import is required for PlatformIO extra scripts
# Import is only available when run by PlatformIO, not in regular Python context
Import("env")  # type: ignore # pylint: disable=undefined-variable


def generate_example_config():
    config_path = "src/config/device_config.h"
    example_path = "src/config/device_config.h.example"

    if not os.path.exists(config_path):
        print(f"Error: {config_path} not found")
        sys.exit(1)

    # Read the main config
    with open(config_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Clean secrets using shared logic
    content = clean_secrets_from_content(content)
    
    # Add example-specific metadata and instructions
    content = add_example_file_metadata(content)

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
    print("🔧 Running pre-build script: generating device_config.h.example...")
    generate_example_config()
