#!/usr/bin/env python3
# pylint: disable=undefined-variable
# type: ignore

import subprocess

Import("env")  # pylint: disable=undefined-variable  # type: ignore


def build_upload_monitor(source, target, env):  # pylint: disable=unused-argument
    """Build, Upload FS & Firmware, Monitor - complete workflow"""
    
    print("🚀 Starting build_upload_monitor workflow...")

    # Resolve the active PlatformIO environment name
    try:
        env_name = env.get("PIOENV") or env.subst("$PIOENV")
    except Exception:  # pylint: disable=broad-except
        env_name = None

    if not env_name:
        print("❌ FAILED: Could not determine current PlatformIO environment")
        return 1

    print(f"🔧 Using PlatformIO env: {env_name}")

    # Step 1/4: Build frontend
    print("\n📦 [1/4] Building frontend...")
    result = subprocess.run(["npm", "run", "build"], check=False)
    if result.returncode != 0:
        print(f"❌ FAILED at step 1/4: Frontend build (exit code {result.returncode})")
        return result.returncode
    print("✅ [1/4] Frontend build completed")

    # Step 2/4: Upload filesystem
    print("\n📁 [2/4] Uploading filesystem...")
    fs_result = env.Execute(f"pio run -e {env_name} -t uploadfs")
    if fs_result != 0:
        print(f"❌ FAILED at step 2/4: Filesystem upload (exit code {fs_result})")
        return fs_result
    print("✅ [2/4] Filesystem upload completed")

    # Step 3/4: Upload firmware (auto-builds if needed)
    print("\n💾 [3/4] Building and uploading firmware...")
    fw_result = env.Execute(f"pio run -e {env_name} -t upload")
    if fw_result != 0:
        print(f"❌ FAILED at step 3/4: Firmware upload (exit code {fw_result})")
        return fw_result
    print("✅ [3/4] Firmware upload completed")

    # Play success sound notification
    try:
        subprocess.run(
            ["/usr/bin/afplay", "-v", "0.8", "/System/Library/Sounds/Glass.aiff"],
            check=False,
        )
    except (subprocess.SubprocessError, FileNotFoundError, OSError):
        pass  # Silent fail for sound

    # Step 4/4: Start monitoring
    print("\n📺 [4/4] Starting serial monitor...")
    env.Execute(f"pio run -e {env_name} -t monitor")
    print("✅ [4/4] All steps completed successfully!")

    return 0


env.AddCustomTarget(  # pylint: disable=undefined-variable  # type: ignore
    "build_upload_monitor",
    None,
    build_upload_monitor,
    "Build, Upload FS & Firmware, Monitor",
    "Build frontend + Upload filesystem + Upload firmware + Start monitor",
)
