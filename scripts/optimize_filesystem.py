"""PlatformIO filesystem optimization script."""

import os
import gzip
import glob
import shutil

# AP mode essential files that must remain uncompressed
ap_essentials = [
    "setup.html",
    "js/page-setup.js",
    "js/app-common.js",
    "js/alpine.js",
    "css/app.css",
]


def build_optimized_filesystem(source, target, pio_env):
    """Copy source files, compress, and remove unnecessary uncompressed versions.

    Args:
        source: Source file (unused, required by PlatformIO)
        target: Target file (unused, required by PlatformIO)
        pio_env: PlatformIO environment object
    """
    # Silence unused parameter warnings
    del source, target

    data_dir = pio_env.get("PROJECT_DATA_DIR", "data")
    src_dir = "src/data"
    file_patterns = ["*.html", "*.png", "*.ico", "*.webmanifest", "*.svg"]
    subdirs = ["js", "css", "settings", "diagnostics", "fonts", "images", "resources"]
    compress_patterns = ["*.html", "css/*.css", "js/*.js", "*.svg", "*.json"]

    # Clean and create data directory
    if os.path.exists(data_dir):
        shutil.rmtree(data_dir)
    os.makedirs(f"{data_dir}/js", exist_ok=True)
    os.makedirs(f"{data_dir}/css", exist_ok=True)

    # Copy files from src/data to data
    print("📂 Copying source files...")

    # Copy individual files
    for pattern in file_patterns:
        for src_file in glob.glob(f"{src_dir}/{pattern}"):
            shutil.copy2(src_file, data_dir)

    # Copy directories
    for subdir in subdirs:
        src_path = f"{src_dir}/{subdir}"
        dst_path = f"{data_dir}/{subdir}"
        if os.path.exists(src_path):
            shutil.copytree(src_path, dst_path, dirs_exist_ok=True)

    print("✓ Source files copied")

    compressed_count = 0
    removed_count = 0

    # Files to compress
    for pattern in compress_patterns:
        for file_path in glob.glob(f"{data_dir}/{pattern}", recursive=True):
            # Skip if already compressed
            if file_path.endswith(".gz"):
                continue

            # Compress the file
            with open(file_path, "rb") as f_in:
                with gzip.open(f"{file_path}.gz", "wb", compresslevel=9) as f_out:
                    f_out.write(f_in.read())
            compressed_count += 1

            # Remove uncompressed if not essential for AP
            relative_path = os.path.relpath(file_path, data_dir)
            if not any(
                relative_path.endswith(essential) for essential in ap_essentials
            ):
                os.remove(file_path)
                removed_count += 1
                print(f"  Removed: {relative_path}")

    print(f"✓ Compressed {compressed_count} files")
    print(f"✓ Removed {removed_count} redundant files")
    print(f"✓ Kept {len(ap_essentials)} AP essentials uncompressed")


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
