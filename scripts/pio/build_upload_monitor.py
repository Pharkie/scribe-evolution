#!/usr/bin/env python3
# pylint: disable=undefined-variable
import subprocess


def build_upload_monitor(source, target, env):  # pylint: disable=unused-argument
    print("🚀 Starting build_upload_monitor workflow...")

    # Determine environment name from env
    env_name = env.get("PIOENV", "esp32c3-dev")

    # Step 1/4: Build frontend
    print("\n📦 [1/4] Building frontend...")
    try:
        fe_result = env.Execute("npm run build")
        if fe_result != 0:
            print(f"❌ FAILED at step 1/4: Frontend build (exit code {fe_result})")
            return
    except Exception as e:
        print(f"❌ Exception during frontend build: {e}")
        return

    # Step 2/4: Upload filesystem (auto-builds if needed)
    print("\n🗂️  [2/4] Uploading filesystem (LittleFS)...")
    fs_result = env.Execute(f"pio run -e {env_name} -t uploadfs")
    if fs_result != 0:
        print(f"❌ FAILED at step 2/4: Filesystem upload (exit code {fs_result})")
        return
    print("✅ [2/4] Filesystem upload completed")

    # Step 3/4: Upload firmware (auto-builds if needed)
    print("\n💾 [3/4] Building and uploading firmware...")
    fw_result = env.Execute(f"pio run -e {env_name} -t upload")
    if fw_result != 0:
        print(f"❌ FAILED at step 3/4: Firmware upload (exit code {fw_result})")
        return
    print("✅ [3/4] Firmware upload completed")

    # Step 4/4: Start serial monitor
    print("\n🛰️  [4/4] Starting serial monitor...")
    # Use upload_port if available, otherwise let pio auto-detect
    upload_port = env.GetProjectOption("upload_port", None)
    monitor_speed = env.GetProjectOption("monitor_speed")

    if upload_port:
        env.Execute(f"pio device monitor -p {upload_port} -b {monitor_speed}")
    else:
        env.Execute(f"pio device monitor -b {monitor_speed}")


Import("env")  # type: ignore # pylint: disable=undefined-variable
env.AddCustomTarget(
    "build_upload_monitor",
    None,
    build_upload_monitor,
    "Build frontend + Upload filesystem + Upload firmware + Start monitor",
)
