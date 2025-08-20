#!/usr/bin/env python3
# pylint: disable=undefined-variable
# type: ignore

import subprocess

Import("env")  # pylint: disable=undefined-variable  # type: ignore


def deploy_all(source, target, env):  # pylint: disable=unused-argument
    """Deploy complete workflow: build + upload fs + upload firmware + monitor"""
    print("🚀 Starting deploy_all workflow...")

    # Step 1: Build frontend
    print("📦 Building frontend...")
    result = subprocess.run(["npm", "run", "build"], check=False)
    if result.returncode != 0:
        print(f"❌ Frontend build failed with exit code {result.returncode}")
        return result.returncode
    print("✅ Frontend build completed")

    # Step 2: Upload filesystem
    print("📁 Uploading filesystem...")
    fs_result = env.Execute("pio run -e main -t uploadfs")
    if fs_result != 0:
        print(f"❌ Filesystem upload failed with exit code {fs_result}")
        return fs_result
    print("✅ Filesystem upload completed")

    # Step 3: Upload firmware (auto-builds if needed)
    print("💾 Building and uploading firmware...")
    fw_result = env.Execute("pio run -e main -t upload")
    if fw_result != 0:
        print(f"❌ Firmware upload failed with exit code {fw_result}")
        return fw_result
    print("✅ Firmware upload completed")

    # Step 4: Start monitoring
    print("📺 Starting serial monitor...")
    env.Execute("pio run -e main -t monitor")

    return 0


env.AddCustomTarget(  # pylint: disable=undefined-variable  # type: ignore
    "deploy_all",
    None,
    deploy_all,
    "Deploy All",
    "Build + Upload FS + Upload Firmware + Monitor",
)
