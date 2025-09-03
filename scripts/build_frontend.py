#!/usr/bin/env python3
"""
Pre-build script to build frontend assets (CSS and JS) before compiling firmware.
This ensures production builds always have the latest frontend assets.
"""

import subprocess
import sys

Import("env")  # pylint: disable=undefined-variable  # type: ignore


def build_frontend(source, target, env):  # pylint: disable=unused-argument
    """Build frontend CSS and JavaScript assets"""
    print("=" * 60)
    print("🎨 BUILDING FRONTEND ASSETS (CSS + JavaScript)")
    print("=" * 60)
    print("Running: npm run build")

    try:
        # Build frontend assets using npm (production builds for minified assets)
        result = subprocess.run(
            ["npm", "run", "build"], check=False, text=True
        )

        if result.returncode == 0:
            print("=" * 60)
            print("✅ FRONTEND BUILD COMPLETED SUCCESSFULLY")
            print("   CSS + JavaScript assets are ready for ESP32")
            print("=" * 60)
        else:
            print("=" * 60)
            print(f"❌ FRONTEND BUILD FAILED (exit code: {result.returncode})")
            print("=" * 60)
            sys.exit(1)

    except FileNotFoundError:
        print("=" * 60)
        print("❌ ERROR: npm not found")
        print("   Please install Node.js and npm first")
        print("=" * 60)
        sys.exit(1)
    except subprocess.SubprocessError as e:
        print("=" * 60)
        print(f"❌ FRONTEND BUILD SUBPROCESS FAILED: {e}")
        print("=" * 60)
        sys.exit(1)


# Run the build function during the pre-build phase
build_frontend(None, None, env)  # pylint: disable=undefined-variable
