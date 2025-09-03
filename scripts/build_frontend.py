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
    print("Building frontend assets...")

    try:
        # Build frontend assets using npm (production builds for minified assets)
        result = subprocess.run(
            ["npm", "run", "build"], check=False, capture_output=True, text=True
        )

        if result.returncode == 0:
            print("Frontend build completed")
        else:
            print(f"Frontend build failed with exit code {result.returncode}")
            if result.stderr:
                print(f"Error: {result.stderr.strip()}")
            sys.exit(1)

    except FileNotFoundError:
        print("npm not found. Please ensure Node.js and npm are installed.")
        sys.exit(1)
    except subprocess.SubprocessError as e:
        print(f"Frontend build subprocess failed: {e}")
        sys.exit(1)


# Run the build function during the pre-build phase
build_frontend(None, None, env)  # pylint: disable=undefined-variable
