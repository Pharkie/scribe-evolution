const fs = require("fs");
const path = require("path");

function loadMockData() {
  const read = (p) =>
    JSON.parse(fs.readFileSync(path.join(__dirname, "..", "data", p), "utf8"));
  return {
    mockConfig: read("mock-config.json"),
    mockConfigAPMode: read("mock-config-ap-mode.json"),
    mockConfigNoLEDs: read("mock-config-no-leds.json"),
    mockDiagnostics: read("mock-diagnostics.json"),
    mockPrinterDiscovery: read("mock-printer-discovery.json"),
    mockNvsDump: read("mock-nvs-dump.json"),
    mockWifiScan: read("mock-wifi-scan.json"),
    mockMemos: read("mock-memos.json"),
    mockRoutes: read("mock-routes.json"),
  };
}

module.exports = { loadMockData };
