# Scribe Evolution Mock Server

A complete mock API server for local development and testing of the Scribe Evolution frontend.

## Files

- **`mock-api.js`** - Main server script
- **`mock-config.json`** - Mock configuration data (matches `/api/config`)
- **`mock-diagnostics.json`** - Mock diagnostics data (matches `/api/diagnostics`)  
- **`mock-printer-discovery.json`** - Mock printer discovery data (matches `/events` SSE)

## Usage

```bash
# Start the server
cd mock-server
node mock-api.js

# Or from project root
node mock-server/mock-api.js
```

## Features

- **Root URL**: `http://localhost:3001/` - serves main interface
- **All API endpoints**: `/api/config`, `/api/diagnostics`, `/api/print`, etc.
- **Server-Sent Events**: `/events` with proper `printer-update` events
- **Static file serving**: HTML, CSS, JS, images, favicons
- **CORS enabled**: for local development
- **Live keyboard controls**:
  - Press **"r" + Enter** to reload server (picks up code changes)
  - Press **"d" + Enter** to reload JSON data files (picks up data changes)
  - Press **Ctrl+C** to stop

## Data Structure

All mock data matches the real ESP32 API responses exactly:

- **Config**: Complete device, MQTT, LED effects, buttons, Unbidden Ink settings
- **Diagnostics**: ESP32-C3 hardware info, memory usage, flash storage, endpoints list
- **Printer Discovery**: SSE events with printer network discovery data

## Development Workflow

1. Edit JSON files to test different data scenarios
2. Press **"d" + Enter** to reload data without restarting server
3. Edit mock-api.js for API behavior changes  
4. Press **"r" + Enter** to reload server completely
5. Frontend sees changes immediately

## Security

All sensitive data (passwords, API keys, real network info) has been replaced with safe mock values.
