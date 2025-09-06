#!/usr/bin/env python3
"""
Build firmware releases for distribution.
This script manages secrets by:
1. Backing up the personal config.h to config.h.adam
2. Generating a clean config.h without secrets
3. Building firmware for multiple targets
4. Copying firmware to organized output directories
5. Restoring the original config.h
"""

import sys
import shutil
import subprocess
import re
from pathlib import Path
import csv

# Add scripts directory to path for importing config_cleaner
sys.path.insert(0, str(Path(__file__).parent))
from config_cleaner import clean_secrets_from_content, add_example_file_metadata


def log(message, level="INFO"):
    """Simple logging function."""
    icons = {"INFO": "🔧", "SUCCESS": "✅", "WARNING": "⚠️", "ERROR": "❌"}
    print(f"{icons.get(level, '•')} {message}")


def is_config_already_clean(config_path):
    """Check if config.h appears to already be cleaned (no secrets)."""
    try:
        with open(config_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Look for placeholder patterns that indicate this is already clean
        placeholder_patterns = [
            r'"YOUR_WIFI_SSID"',
            r'"YOUR_WIFI_PASSWORD"',
            r'"YOUR_MQTT_SERVER',
            r'"YOUR_MQTT_USERNAME"',
            r'"YOUR_MQTT_PASSWORD"',
            r'"YOUR_DEVICE_NAME"',
            r'"YOUR_OPENAI_API_KEY"',
            r'"YOUR_BETTERSTACK_TOKEN"',
        ]

        placeholder_count = 0
        for pattern in placeholder_patterns:
            if re.search(pattern, content):
                placeholder_count += 1

        # If we find multiple placeholders, this config is likely already clean
        return placeholder_count >= 3

    except Exception:
        return False


def backup_config():
    """Backup the current config.h to config.h.adam, with safety checks."""
    config_path = Path("src/core/config.h")
    backup_path = Path("src/core/config.h.adam")

    if not config_path.exists():
        log(f"Error: {config_path} not found", "ERROR")
        sys.exit(1)

    # Check if current config appears to already be cleaned
    if is_config_already_clean(config_path):
        log(
            "⚠️  Current config.h appears to already be cleaned (contains placeholders)",
            "WARNING",
        )

        if backup_path.exists():
            log("Found existing config.h.adam backup - will use that instead", "INFO")
            log(
                "Current config.h will be overwritten with clean version for build",
                "INFO",
            )
            return True
        else:
            log("❌ No backup found and current config appears clean!", "ERROR")
            log("❌ Cannot proceed - would lose your original configuration", "ERROR")
            log(
                "💡 Please restore your original config.h with secrets and try again",
                "ERROR",
            )
            sys.exit(1)

    # Check if we're about to overwrite a good backup with a bad one
    if backup_path.exists():
        if is_config_already_clean(backup_path):
            log(
                "⚠️  Existing backup also appears to be cleaned - creating timestamped backup",
                "WARNING",
            )
            import datetime

            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            timestamped_backup = Path(f"src/core/config.h.adam.{timestamp}")
            try:
                shutil.copy2(config_path, timestamped_backup)
                log(f"Created timestamped backup: {timestamped_backup}", "SUCCESS")
            except Exception as e:
                log(f"Failed to create timestamped backup: {e}", "WARNING")
        else:
            log("Existing backup appears to contain secrets - keeping it safe", "INFO")
            return True  # Don't overwrite good backup

    try:
        shutil.copy2(config_path, backup_path)
        log(f"Backed up {config_path} to {backup_path}", "SUCCESS")
        return True
    except Exception as e:
        log(f"Failed to backup config: {e}", "ERROR")
        return False


def restore_config():
    """Restore the original config.h from config.h.adam."""
    config_path = Path("src/core/config.h")
    backup_path = Path("src/core/config.h.adam")

    if not backup_path.exists():
        log(f"Warning: No backup found at {backup_path}", "WARNING")
        return False

    try:
        shutil.copy2(backup_path, config_path)
        log(f"Restored {config_path} from {backup_path}", "SUCCESS")
        return True
    except Exception as e:
        log(f"Failed to restore config: {e}", "ERROR")
        return False


def generate_clean_config():
    """Generate config.h.example with secrets cleaned, then replace config.h with it."""
    config_path = Path("src/core/config.h")
    example_path = Path("src/core/config.h.example")

    if not config_path.exists():
        log(f"Error: {config_path} not found", "ERROR")
        return False

    try:
        # Read the main config
        with open(config_path, "r", encoding="utf-8") as f:
            content = f.read()

        log("Cleaning secrets from configuration...", "INFO")

        # Clean secrets using shared logic
        cleaned_content = clean_secrets_from_content(content)

        # Check if anything was actually cleaned
        if content == cleaned_content:
            log("Warning: No secrets were detected/cleaned", "WARNING")
        else:
            log("Secrets successfully cleaned from configuration", "SUCCESS")

        # Step 1: Write clean config to .example file (with example metadata/instructions)
        example_content = add_example_file_metadata(cleaned_content)
        with open(example_path, "w", encoding="utf-8") as f:
            f.write(example_content)
        log(f"Generated {example_path} with cleaned secrets", "SUCCESS")

        # Step 2: Replace config.h with the clean version for build
        with open(config_path, "w", encoding="utf-8") as f:
            f.write(cleaned_content)
        log(f"Replaced {config_path} with clean version for build", "SUCCESS")
        
        return True

    except Exception as e:
        log(f"Failed to generate clean config: {e}", "ERROR")
        log(f"Exception details: {type(e).__name__}: {str(e)}", "ERROR")
        return False


def build_firmware(environment):
    """Build firmware for a specific PlatformIO environment."""
    log(f"Building firmware for {environment}...")

    try:
        # Run platformio build
        result = subprocess.run(
            ["pio", "run", "-e", environment],
            capture_output=True,
            text=True,
            check=True,
        )

        log(f"Successfully built firmware for {environment}", "SUCCESS")
        return True

    except subprocess.CalledProcessError as e:
        log(f"Build failed for {environment}:", "ERROR")
        log(f"Return code: {e.returncode}", "ERROR")
        if e.stdout:
            log(f"STDOUT: {e.stdout}", "ERROR")
        if e.stderr:
            log(f"STDERR: {e.stderr}", "ERROR")
        return False
    except FileNotFoundError:
        log("PlatformIO not found. Please install it: pip install platformio", "ERROR")
        return False


def copy_firmware(environment):
    """Copy the built firmware, bootloader, and partitions and create merged binary."""
    firmware_source = Path(f".pio/build/{environment}/firmware.bin")
    firmware_dest = Path(f"firmware/{environment}/firmware.bin")
    
    bootloader_source = Path(f".pio/build/{environment}/bootloader.bin")
    bootloader_dest = Path(f"firmware/{environment}/bootloader.bin")
    
    partitions_source = Path(f".pio/build/{environment}/partitions.bin")
    partitions_dest = Path(f"firmware/{environment}/partitions.bin")
    
    merged_dest = Path(f"firmware/{environment}/scribe-{environment}-complete.bin")

    if not firmware_source.exists():
        log(f"Warning: Firmware not found at {firmware_source}", "WARNING")
        return False

    try:
        # Ensure destination directory exists
        firmware_dest.parent.mkdir(parents=True, exist_ok=True)

        # Copy individual files (for advanced users)
        shutil.copy2(firmware_source, firmware_dest)
        size = firmware_dest.stat().st_size
        size_kb = size / 1024
        log(f"Copied firmware to {firmware_dest} ({size_kb:.1f} KB)", "SUCCESS")
        
        if bootloader_source.exists():
            shutil.copy2(bootloader_source, bootloader_dest)
            log(f"Copied bootloader to {bootloader_dest}", "SUCCESS")
        else:
            log(f"Warning: Bootloader not found at {bootloader_source}", "WARNING")
            return False
        
        if partitions_source.exists():
            shutil.copy2(partitions_source, partitions_dest)
            log(f"Copied partition table to {partitions_dest}", "SUCCESS")
        else:
            log(f"Warning: Partition table not found at {partitions_source}", "WARNING")
            return False

        return True

    except Exception as e:
        log(f"Failed to copy firmware for {environment}: {e}", "ERROR")
        return False


def _get_partitions_file_for_env(environment: str) -> Path:
    """Return the partitions CSV path for a given PlatformIO environment by reading platformio.ini.

    Falls back to partitions_no_ota.csv if not found.
    """
    ini_path = Path("platformio.ini")
    default = Path("partitions_no_ota.csv")
    try:
        content = ini_path.read_text(encoding="utf-8")
        # crude parse: find section header then look for board_build.partitions = file
        import re
        pattern = rf"\[env:{re.escape(environment)}\][\s\S]*?board_build\.partitions\s*=\s*(.+)"
        m = re.search(pattern, content)
        if m:
            candidate = Path(m.group(1).strip())
            if candidate.exists():
                return candidate
    except Exception:
        pass
    return default


def _get_fs_offset_from_partitions(csv_path: Path) -> str:
    """Parse the partitions CSV and return the LittleFS filesystem offset as hex string (e.g., '0x210000')."""
    try:
        with csv_path.open("r", encoding="utf-8") as f:
            reader = csv.reader(f)
            for row in reader:
                if not row or row[0].strip().startswith("#"):
                    continue
                # Expect: name, type, subtype, offset, size
                if len(row) >= 5:
                    name = row[0].strip().lower()
                    offset = row[3].strip()
                    if name == "littlefs":
                        # Normalize to 0x... string
                        if not offset.startswith("0x"):
                            offset = hex(int(offset, 0))
                        return offset
        # If we get here, no littlefs partition was found - fail fast
        log(f"FATAL: No 'littlefs' partition found in {csv_path}", "ERROR")
        log("Expected partition name must be exactly 'littlefs'", "ERROR")
        raise SystemExit(1)
    except Exception as e:
        log(f"Failed to parse partitions file {csv_path}: {e}", "ERROR")
        raise SystemExit(1)


def create_merged_binary(environment):
    """Create a merged binary file containing all components for easy flashing."""
    bootloader_source = Path(f".pio/build/{environment}/bootloader.bin")
    partitions_source = Path(f".pio/build/{environment}/partitions.bin")
    firmware_source = Path(f".pio/build/{environment}/firmware.bin")
    littlefs_source = Path(f".pio/build/{environment}/littlefs.bin")
    
    merged_dest = Path(f"firmware/{environment}/scribe-{environment}-complete.bin")
    
    # Check all required files exist (no boot_app0.bin needed - we're not using OTA)
    required_files = [bootloader_source, partitions_source, firmware_source, littlefs_source]
    for file in required_files:
        if not file.exists():
            log(f"Warning: Required file not found: {file}", "WARNING")
            return False
    
    try:
        # Create merged binary using esptool merge-bin
        log(f"Creating complete merged binary for {environment}...", "INFO")

        # Determine chip type and bootloader address for merge-bin command
        if environment.startswith("esp32c3"):
            chip_type = "ESP32C3"
            bootloader_addr = "0x0000"  # ESP32-C3 uses 0x0000
        else:
            chip_type = "ESP32" 
            bootloader_addr = "0x1000"  # Original ESP32 uses 0x1000
        # Determine filesystem offset from the environment's partitions file
        partitions_csv = _get_partitions_file_for_env(environment)
        fs_offset = _get_fs_offset_from_partitions(partitions_csv)

        merge_cmd = [
            "esptool", "--chip", chip_type, "merge-bin", 
            "-o", str(merged_dest),
            "--flash-mode", "dio", "--flash-size", "4MB",
            bootloader_addr, str(bootloader_source),
            "0x8000", str(partitions_source), 
            "0x10000", str(firmware_source),
            fs_offset, str(littlefs_source)
        ]
        
        result = subprocess.run(merge_cmd, capture_output=True, text=True, check=True)
        
        merged_size = merged_dest.stat().st_size
        merged_size_kb = merged_size / 1024
        log(f"Created complete binary: {merged_dest} ({merged_size_kb:.1f} KB)", "SUCCESS")

        return True

    except subprocess.CalledProcessError as e:
        log(f"Failed to create merged binary: {e}", "ERROR")
        if e.stderr:
            log(f"Error details: {e.stderr}", "ERROR")
        return False
    except Exception as e:
        log(f"Failed to create merged binary: {e}", "ERROR")
        return False


def build_filesystem(environment):
    """Build and copy filesystem image for environment."""
    log(f"Building filesystem for {environment}...")

    try:
        # Run platformio filesystem build
        result = subprocess.run(
            ["pio", "run", "--target", "buildfs", "-e", environment],
            capture_output=True,
            text=True,
            check=True,
        )

        # Copy the filesystem image
        fs_source = Path(f".pio/build/{environment}/littlefs.bin")
        fs_dest = Path(f"firmware/{environment}/littlefs.bin")

        if fs_source.exists():
            shutil.copy2(fs_source, fs_dest)
            size = fs_dest.stat().st_size
            size_kb = size / 1024
            log(f"Copied filesystem to {fs_dest} ({size_kb:.1f} KB)", "SUCCESS")
        else:
            log(f"Warning: Filesystem image not found at {fs_source}", "WARNING")

        return True

    except subprocess.CalledProcessError as e:
        log(f"Filesystem build failed for {environment}: {e}", "ERROR")
        return False


def create_release_info():
    """Create release information files in firmware directories."""
    import datetime
    
    # Load template from file
    template_path = Path(__file__).parent / "templates" / "firmware_readme.md"
    try:
        with open(template_path, "r", encoding="utf-8") as f:
            template = f.read()
        # Prefer macOS default port in README examples
        template = template.replace("/dev/ttyUSB0", "/dev/cu.usbmodem1201")
    except FileNotFoundError:
        log(f"Warning: Template not found at {template_path}", "WARNING")
        log("Creating basic README without template", "WARNING")
        template = "# Scribe Firmware Release\n\nBuilt on: {build_date}\n\nSee /docs/quick-start.md for flashing instructions."
    
    # Format template with build date and environment
    build_date = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    for env in ["esp32c3-prod", "esp32c3-prod-no-leds", "lolin32lite-no-leds"]:
        # Format template for this specific environment
        release_info = template.format(build_date=build_date, environment=env)
        
        info_file = Path(f"firmware/{env}/README.md")
        try:
            with open(info_file, "w", encoding="utf-8") as f:
                f.write(release_info)
            log(f"Created release info: {info_file}", "SUCCESS")
        except Exception as e:
            log(f"Failed to create release info for {env}: {e}", "WARNING")


def recover_config():
    """Emergency recovery - restore config.h from backup."""
    backup_path = Path("src/core/config.h.adam")
    config_path = Path("src/core/config.h")

    if not backup_path.exists():
        log("❌ No backup found at src/core/config.h.adam", "ERROR")
        return False

    if is_config_already_clean(backup_path):
        log(
            "⚠️  Backup also appears to be cleaned - check for timestamped backups:",
            "WARNING",
        )
        backup_dir = Path("src/core")
        timestamped_backups = list(backup_dir.glob("config.h.adam.*"))
        if timestamped_backups:
            log("Found timestamped backups:", "INFO")
            for backup in sorted(timestamped_backups):
                log(f"  - {backup}", "INFO")
            log("You may want to manually restore from one of these", "INFO")
        return False

    try:
        shutil.copy2(backup_path, config_path)
        log(f"✅ Recovered {config_path} from {backup_path}", "SUCCESS")
        return True
    except Exception as e:
        log(f"❌ Failed to recover config: {e}", "ERROR")
        return False


def main():
    """Main build process."""
    # Check for recovery mode
    if len(sys.argv) > 1 and sys.argv[1] == "--recover":
        log("🔧 Running in recovery mode...")
        if recover_config():
            log("🎉 Recovery completed successfully!", "SUCCESS")
        else:
            log("❌ Recovery failed", "ERROR")
        return

    log("Starting firmware release build process...")

    # Check we're in the right directory
    if not Path("platformio.ini").exists():
        log("Error: Not in a PlatformIO project directory", "ERROR")
        sys.exit(1)

    # Build targets
    targets = ["esp32c3-prod", "esp32c3-prod-no-leds", "lolin32lite-no-leds"]

    success = True

    try:
        # Step 1: Backup current config
        if not backup_config():
            sys.exit(1)

        # Step 2: Generate clean config
        if not generate_clean_config():
            success = False

        if success:
            # Step 3: Build frontend assets first
            log("Building frontend assets...")
            try:
                result = subprocess.run(["npm", "run", "build"], check=True, capture_output=True, text=True)
                log("Frontend assets built successfully", "SUCCESS")
            except subprocess.CalledProcessError as e:
                log("❌ Frontend build failed - this is a fatal error", "ERROR")
                log(f"❌ Return code: {e.returncode}", "ERROR")
                if e.stdout:
                    log(f"❌ STDOUT: {e.stdout}", "ERROR")
                if e.stderr:
                    log(f"❌ STDERR: {e.stderr}", "ERROR")
                log("❌ Cannot continue without frontend assets", "ERROR")
                success = False
            except FileNotFoundError:
                log("❌ npm not found - frontend build is required", "ERROR")
                log("❌ Please install Node.js and npm", "ERROR")
                success = False

            # Step 4: Build firmware for each target (only if frontend succeeded)
            if success:
                for target in targets:
                    log(f"\\n--- Building {target} ---")

                    # Build firmware
                    if build_firmware(target):
                        copy_firmware(target)
                        build_filesystem(target)
                        # Create merged binary after filesystem is built
                        create_merged_binary(target)
                    else:
                        success = False
            else:
                log("⏭️  Skipping firmware builds due to frontend failure", "WARNING")

        # Step 5: Create release information
        create_release_info()

    finally:
        # Step 6: Always restore original config
        restore_config()

    if success:
        log("\\n🎉 Firmware release build completed successfully!", "SUCCESS")
        log("\\nFirmware files are available in:")
        for target in targets:
            firmware_path = Path(f"firmware/{target}/firmware.bin")
            if firmware_path.exists():
                size = firmware_path.stat().st_size / 1024
                log(f"  • firmware/{target}/ ({size:.1f} KB)")
    else:
        log("\\n❌ Firmware release build failed", "ERROR")
        sys.exit(1)


if __name__ == "__main__":
    main()
