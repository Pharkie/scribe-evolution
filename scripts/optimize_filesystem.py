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

    # Clean and create data directory
    if os.path.exists(data_dir):
        if os.path.isdir(data_dir):
            shutil.rmtree(data_dir)
        else:
            os.remove(data_dir)  # Remove if it's a file
    os.makedirs(data_dir, exist_ok=True)

    # Copy files from src/data to data
    print("📂 Copying source files...")

    # Copy individual files
    for pattern in file_patterns:
        for src_file in glob.glob(f"{src_dir}/{pattern}"):
            shutil.copy2(src_file, data_dir)

    # Copy all directories (dynamic discovery)
    for item in os.listdir(src_dir):
        src_path = f"{src_dir}/{item}"
        if os.path.isdir(src_path):
            dst_path = f"{data_dir}/{item}"
            shutil.copytree(src_path, dst_path, dirs_exist_ok=True)

    print("✓ Source files copied")

    compressed_count = 0
    removed_count = 0

    # Compressible file extensions
    compressible_extensions = {".html", ".css", ".js", ".svg", ".json"}

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
