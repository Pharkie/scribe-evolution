const fs = require("fs");
const path = require("path");

// Fail fast: if any required mock file is missing or invalid, throw.
function strictRead(file) {
  const full = path.join(__dirname, "..", "data", file);
  const text = fs.readFileSync(full, "utf8");
  return JSON.parse(text);
}

function loadMockData() {
  return {
    mockConfig: strictRead("mock-config.json"),
    mockConfigAPMode: strictRead("mock-config-ap-mode.json"),
    mockConfigNoLEDs: strictRead("mock-config-no-leds.json"),
    mockDiagnostics: strictRead("mock-diagnostics.json"),
    mockPrinterDiscovery: strictRead("mock-printer-discovery.json"),
    mockNvsDump: strictRead("mock-nvs-dump.json"),
    mockWifiScan: strictRead("mock-wifi-scan.json"),
    mockMemos: strictRead("mock-memos.json"),
    mockRoutes: strictRead("mock-routes.json"),
  };
}

module.exports = { loadMockData };
