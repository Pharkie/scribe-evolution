# Scribe Evolution Mock Server

A complete mock API server for local development and testing of the Scribe Evolution frontend.

## Why Use the Mock Server?

**Rapid Development**: Test frontend changes without rebuilding and flashing ESP32 firmware (saves 30-60 seconds per iteration)

**Safe Testing**: Experiment with UI/UX without risk of corrupting device settings or requiring hardware reset

**Complete Coverage**: Every API endpoint accurately simulated with realistic response times and data structures

**Live Data**: Dynamic values (uptime, memory, temperature) that change over time for realistic testing

**No Hardware Required**: Develop and test the complete web interface on any machine with Node.js

## Files

- **`mock-api.js`** - Main server script with enhanced port conflict resolution
- **`data/`** - Mock data files directory
  - **`mock-config.json`** - Normal mode configuration (matches `/api/config`)
  - **`mock-config-ap-mode.json`** - AP mode configuration for setup testing
  - **`mock-config-no-leds.json`** - No-LEDs build configuration
  - **`mock-diagnostics.json`** - Mock diagnostics data (matches `/api/diagnostics`)
  - **`mock-printer-discovery.json`** - Mock printer discovery data (matches `/events` SSE)
  - **`mock-nvs-dump.json`** - Mock NVS storage dump (matches `/api/nvs-dump`)
  - **`mock-wifi-scan.json`** - Mock WiFi network scan results
  - **`mock-memos.json`** - Mock memo storage data

## Usage

```bash
# Start the server (normal mode)
cd mock-server
node mock-api.js

# Start in AP mode (for testing setup.html)
node mock-api.js --ap-mode

# Start in no-LEDs mode (for testing builds without LED support)
node mock-api.js --no-leds

# Or from project root
node mock-server/mock-api.js [--ap-mode|--no-leds]
```

## Features

- **Complete API Coverage**: All ESP32 endpoints with realistic response times
  - `GET /api/config` - Device configuration with live data updates and masking secrets
  - `POST /api/config` - Configuration updates
  - `GET /api/diagnostics` - System diagnostics with live memory/temperature
  - `GET /api/nvs-dump` - Raw NVS storage dump with timestamp updates
  - `GET /api/status` - System status endpoint
  - `POST /api/print` - Print job simulation with character counts
  - `POST /api/led-effect` - LED effect triggering
- **Server-Sent Events**: `/events` with proper `printer-update` events and discovery data
- **Static File Serving**: Complete web interface (HTML, CSS, JS, images, favicons)
- **CORS Enabled**: Cross-origin requests for development tools
- **Smart Port Conflict Resolution**: Automatically detects port 3001 conflicts and offers to kill conflicting processes
- **Mode Switching**: `--ap-mode` for testing setup.html, `--no-leds` for LED-disabled builds
- **Live Keyboard Controls**:
  - Press **"r" + Enter** to reload server (picks up code changes)
  - Press **"d" + Enter** to reload JSON data files (picks up data changes)
  - Press **Ctrl+C** to stop server

## Data Structure

All mock data matches the real ESP32 API responses exactly:

- **Config**: Complete device settings, MQTT configuration, LED effects, button mappings, Unbidden Ink parameters
- **Diagnostics**: ESP32-C3 hardware specifications, live memory usage, flash storage, temperature, endpoint listings
- **Printer Discovery**: SSE events with network printer discovery data and connection timestamps
- **NVS Dump**: Complete non-volatile storage dump with all 35 configuration keys, validation status, and data types

## Development Workflow

1. **Start the server**: `cd mock-server && node mock-api.js`
2. **Access web interface**: http://localhost:3001/
3. **Edit JSON files** to test different data scenarios
4. **Press "d" + Enter** to reload data without restarting server
5. **Edit mock-api.js** for API behavior changes
6. **Press "r" + Enter** to reload server completely
7. **Frontend sees changes immediately** - no ESP32 rebuild required

## Available Endpoints

| Endpoint           | Method | Description          | Live Data             |
| ------------------ | ------ | -------------------- | --------------------- |
| `/`                | GET    | Main web interface   | Static                |
| `/api/config`      | GET    | Device configuration | ✓                     |
| `/api/config`      | POST   | Config updates       | Simulated             |
| `/api/diagnostics` | GET    | System diagnostics   | ✓ Memory, temperature |
| `/api/nvs-dump`    | GET    | NVS storage dump     | ✓ Timestamp           |
| `/api/status`      | GET    | System status        | ✓                     |
| `/api/print`       | POST   | Print simulation     | Random counts         |
| `/api/led-effect`  | POST   | LED effect trigger   | Simulated             |
| `/events`          | SSE    | Printer discovery    | ✓ Periodic updates    |

## Testing Scenarios

**Secret Masking**: Test that WiFi passwords, MQTT passwords, and ChatGPT API tokens display as masked values (`mo●●●●●●●●en`) in the settings interface

**Password Updates**: Test that only modified passwords are submitted to the server, while unchanged masked values are preserved

**Configuration Changes**: Modify `mock-config.json` to test different device setups, MQTT configurations, or LED effects

**System Diagnostics**: Adjust memory values in `mock-diagnostics.json` to test low-memory warnings or system alerts

**Network Discovery**: Update printer discovery data to test multi-printer scenarios or network issues

**NVS Storage**: Modify storage dump to test different configuration states, validation errors, or missing keys

## Security

All sensitive data (passwords, API keys, real network info) has been replaced with safe mock values that demonstrate the new secret masking functionality:

- WiFi passwords: `mo●●●●●●●●en` (shows first 2 and last 2 characters with masking)
- MQTT passwords: `mo●●●●●●●●rd` (demonstrates consistent masking pattern)
- ChatGPT API tokens: `sk●●●●●●●●ty` (masks sensitive API key data)
