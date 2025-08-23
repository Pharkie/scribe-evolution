# ESP32-C3 Development Aliases
# Add these to your ~/.zshrc or run them directly

# Quick ESP32 connectivity check (2-5 seconds)
alias esp32-check='python3 "$PWD/scripts/check_esp32.py"'

# Super fast port check (instant)
alias esp32-port='ls -la /dev/cu.usb* 2>/dev/null || echo "No USB devices"'

# List all PlatformIO devices
alias esp32-list='pio device list'

# Quick upload without monitoring (faster)
alias esp32-upload='pio run --target upload -e main'

# Upload + monitor in one command  
alias esp32-flash='pio run --target upload -e main && pio device monitor'

# Just monitor (no upload)
alias esp32-monitor='pio device monitor'

# Build without upload (faster feedback)
alias esp32-build='pio run -e main'

# Clean build
alias esp32-clean='pio run --target clean -e main'
