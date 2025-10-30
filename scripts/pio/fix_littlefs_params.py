"""
PlatformIO Build Script: Fix LittleFS Block/Page Size Mismatch

Problem: mklittlefs uses different default block/page sizes than esp_littlefs component
Solution: Override mklittlefs parameters to match ESP-IDF defaults

ESP-IDF esp_littlefs defaults:
- Block size: 4096 bytes
- Page size: 256 bytes
- Lookahead: 128 bytes

Standard mklittlefs defaults (WRONG for ESP32):
- Block size: 4096 bytes
- Page size: 256 bytes (usually correct)

This script ensures mklittlefs creates an image compatible with ESP32's runtime mounting.
"""

Import("env")

def override_littlefs_params(source, target, env):
    """Override mklittlefs parameters before filesystem build"""
    print("=" * 60)
    print("Configuring LittleFS with ESP32-compatible parameters:")
    print("  Block Size: 4096 bytes")
    print("  Page Size:  256 bytes")
    print("=" * 60)

    # Set filesystem build flags
    # These are passed to mklittlefs tool
    env.Replace(
        MKFSTOOL="mklittlefs",
        MKFSFLAGS=[
            "-b", "4096",  # Block size (must match CONFIG_LITTLEFS_BLOCK_SIZE)
            "-p", "256",   # Page size (must match CONFIG_LITTLEFS_PAGE_SIZE)
        ]
    )

# Register the override before buildfs target
env.AddPreAction("$BUILD_DIR/littlefs.bin", override_littlefs_params)

print("[fix_littlefs_params.py] LittleFS parameter override script loaded")
