"""PlatformIO filesystem optimization script."""

import os
import gzip
import glob
import shutil

# Uncompressed files to keep on device filesystem
# - AP essentials (captive portal needs uncompressed HTML/CSS/JS)
# - GitHub-visible assets (README references uncompressed .svg logos)
ap_essentials = [
    "setup.html",
    "js/page-setup.js",
    "js/app-common.js",
    "js/alpine.js",
    "css/app.css",
    # Keep manifest uncompressed for AP mode (served without gzip)
    "site.webmanifest",
]

readme_assets = [
    "images/ScribeLogoMain-black.svg",
    "images/ScribeLogoMain-white.svg",
]

keep_uncompressed = set(ap_essentials + readme_assets)


def build_optimized_filesystem(source, target, pio_env):
    """Optimize existing data directory: copy static assets, compress files, protect AP essentials.

    Args:
        source: Source file (unused, required by PlatformIO)
        target: Target file (unused, required by PlatformIO)
        pio_env: PlatformIO environment object
    """
    # Silence unused parameter warnings
    del source, target

    data_dir = pio_env.get("PROJECT_DATA_DIR", "data")
    src_dir = "src/web-static"
    file_patterns = ["*.html", "*.png", "*.ico", "*.webmanifest", "*.svg"]

    # Ensure data directory exists (but don't wipe it)
    os.makedirs(data_dir, exist_ok=True)

    # Copy/update static assets from src/web-static to data
    print("📂 Copying static assets...")

    # Copy individual files
    for pattern in file_patterns:
        for src_file in glob.glob(f"{src_dir}/{pattern}"):
            dest_file = os.path.join(data_dir, os.path.basename(src_file))
            shutil.copy2(src_file, dest_file)

    # Copy all directories (dynamic discovery)
    if os.path.exists(src_dir):
        for item in os.listdir(src_dir):
            src_path = os.path.join(src_dir, item)
            if os.path.isdir(src_path):
                dst_path = os.path.join(data_dir, item)
                # Remove existing directory first to ensure clean copy
                if os.path.exists(dst_path):
                    shutil.rmtree(dst_path)
                shutil.copytree(src_path, dst_path)

    print("✓ Static assets copied")

    # Copy warning file from template
    warning_template = "scripts/templates/data_warning.md"
    warning_dest = os.path.join(data_dir, "AGENTS.md")
    if os.path.exists(warning_template):
        shutil.copy2(warning_template, warning_dest)
        print("✓ Warning file (AGENTS.md) copied from template")

    # Now optimize the live directory: compress files and clean up
    print("⚡ Optimizing filesystem...")
    
    compressed_count = 0
    removed_count = 0

    # Compressible file extensions
    # Include .webmanifest so STA mode can serve gzipped manifest
    compressible_extensions = {".html", ".css", ".js", ".svg", ".json", ".webmanifest", ".txt"}

    # Walk through all files recursively
    for root, _, files in os.walk(data_dir):
        for file_name in files:
            file_path = os.path.join(root, file_name)
            
            # Skip if already compressed
            if file_path.endswith(".gz"):
                continue
                
            # Check if file should be compressed based on extension
            file_ext = os.path.splitext(file_name)[1].lower()
            if file_ext in compressible_extensions:
                # Create compressed version
                gz_path = f"{file_path}.gz"
                
                # Only compress if .gz doesn't exist or source is newer
                should_compress = True
                if os.path.exists(gz_path):
                    src_mtime = os.path.getmtime(file_path)
                    gz_mtime = os.path.getmtime(gz_path)
                    should_compress = src_mtime > gz_mtime
                
                if should_compress:
                    with open(file_path, "rb") as f_in:
                        with gzip.open(gz_path, "wb", compresslevel=9) as f_out:
                            f_out.write(f_in.read())
                    compressed_count += 1

                # Remove uncompressed version if not essential for AP mode
                relative_path = os.path.relpath(file_path, data_dir)
                is_protected = any(
                    relative_path == essential or relative_path.endswith(f"/{essential}")
                    for essential in keep_uncompressed
                )
                
                if not is_protected:
                    os.remove(file_path)
                    removed_count += 1
                    print(f"  Removed: {relative_path}")

    print(f"✓ Compressed {compressed_count} files")
    print(f"✓ Removed {removed_count} redundant uncompressed files")
    # Report kept groups
    print(f"✓ Kept {len(ap_essentials)} AP essentials uncompressed")
    print(f"✓ Kept {len(readme_assets)} README assets uncompressed")


# Can be called directly or from PlatformIO
def main():
    """Run filesystem optimization directly (not from PlatformIO)."""
    # Create minimal environment object
    class MockEnv:
        def get(self, key, default):
            del key  # Silence unused parameter warning
            return default
    
    mock_env = MockEnv()
    build_optimized_filesystem(None, None, mock_env)


# PlatformIO import and hook - handled at runtime
try:
    Import("env")  # type: ignore # pylint: disable=undefined-variable
    env.AddPreAction("$BUILD_DIR/littlefs.bin", build_optimized_filesystem)  # type: ignore # pylint: disable=undefined-variable
except NameError:
    # Running outside PlatformIO context
    if __name__ == "__main__":
        main()

