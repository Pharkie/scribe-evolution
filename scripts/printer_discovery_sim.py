#!/usr/bin/env python3
"""
Printer Discovery Simulator / Demo (renamed from test_printer_discovery.py)

Usage:
  python3 scripts/printer_discovery_sim.py --scenario home

This thin wrapper calls the original implementation in test_printer_discovery.py
to preserve backwards compatibility while clarifying that this is a demo/simulator,
not a unit test.
"""

import sys


def main():
    try:
        # Defer import to avoid any global side effects when listing help
        from test_printer_discovery import main as legacy_main  # type: ignore
    except Exception as e:
        print("❌ Unable to import legacy test_printer_discovery.py:", e)
        print("   Make sure this script is run from the project root.")
        sys.exit(1)
    # Delegate to the original script's CLI
    legacy_main()


if __name__ == "__main__":
    main()

