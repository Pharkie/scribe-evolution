#!/usr/bin/env python3
"""
Custom PlatformIO target for complete deploy workflow:
1. Build frontend assets (npm run build)
2. Upload filesystem
3. Upload firmware
4. Start monitor
"""

Import("env")

import subprocess
import os


def deploy_all(source, target, env):
    """Complete deployment workflow"""
    print("🚀 Starting complete deployment workflow...")

    # Step 1: Build frontend assets
    print("🎨 Building frontend assets...")
    try:
        subprocess.run(["npm", "run", "build"], check=True, cwd=os.getcwd())
        print("✅ Frontend assets built successfully!")
    except subprocess.CalledProcessError as e:
        print(f"❌ Frontend build failed: {e}")
        env.Exit(1)
    except FileNotFoundError:
        print("❌ npm not found. Please install Node.js and npm.")
        env.Exit(1)

    # Step 2: Upload filesystem
    print("📁 Uploading filesystem...")
    fs_result = env.Execute("pio run --environment main --target uploadfs")
    if fs_result != 0:
        print("❌ Filesystem upload failed!")
        env.Exit(1)
    print("✅ Filesystem uploaded successfully!")

    # Step 3: Upload firmware
    print("💾 Uploading firmware...")
    fw_result = env.Execute("pio run --environment main --target upload")
    if fw_result != 0:
        print("❌ Firmware upload failed!")
        env.Exit(1)
    print("✅ Firmware uploaded successfully!")

    # Step 4: Start monitor
    print("📺 Starting serial monitor...")
    print("🔌 Press Ctrl+C to exit monitor")
    try:
        env.Execute("pio run --environment main --target monitor")
    except KeyboardInterrupt:
        print("✅ Monitor stopped by user")

    print("✅ Complete deployment finished!")


# Add the custom target
env.AddCustomTarget(
    name="deploy_all",
    dependencies=None,
    actions=[deploy_all],
    title="Deploy All: Build Frontend + Upload FS + Upload Firmware + Monitor",
    description="Complete workflow: build frontend assets, upload filesystem, upload firmware, and start monitoring",
)
