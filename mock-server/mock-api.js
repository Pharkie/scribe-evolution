#!/usr/bin/env node

/**
 * Mock API Server for Scribe Evolution Local Development
 * Minimal bootstrap that delegates to handlers and utils.
 * Usage: node mock-api.js [--ap-mode|--no-leds|--disable-mqtt]
 */

const http = require("http");
const fs = require("fs");
const path = require("path");
const { URL } = require("url");

const { sendNotFound, serveFileOr404, serveText } = require("./utils/respond");
const { handleAPI } = require("./handlers/api");

const PORT = 3001;
const startTime = Date.now();

// Determine mode from CLI args
const args = process.argv.slice(2);
const currentMode = args.includes("--ap-mode")
  ? "ap-mode"
  : args.includes("--no-leds")
  ? "no-leds"
  : args.includes("--disable-mqtt")
  ? "disable-mqtt"
  : "normal";

// =============================
// Validation helpers
// =============================

const VALIDATION_TYPES = {
  STRING: "string",
  NON_EMPTY_STRING: "non_empty_string",
  IANA_TIMEZONE: "iana_timezone",
  GPIO: "gpio",
  RANGE_INT: "range_int",
  BOOLEAN: "boolean",
  ENUM_STRING: "enum_string",
};

const VALID_LED_EFFECTS = [
  "chase_single",
  "chase_multi",
  "rainbow",
  "twinkle",
  "pulse",
  "matrix",
  "none",
];

const VALID_GPIOS = [ -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 21 ];
const SAFE_GPIOS  = [ -1, 2, 4, 5, 6, 7, 10, 20, 21 ];

function validateField(fieldPath, value, fieldDef) {
  switch (fieldDef.type) {
    case VALIDATION_TYPES.STRING:
      return { valid: typeof value === "string" };
    case VALIDATION_TYPES.NON_EMPTY_STRING:
      if (typeof value !== "string") return { valid: false, error: `${fieldPath} must be a string` };
      if (!value.length) return { valid: false, error: `${fieldPath} cannot be empty` };
      return { valid: true };
    case VALIDATION_TYPES.IANA_TIMEZONE: {
      if (typeof value !== "string" || !value.length) {
        return { valid: false, error: `${fieldPath} must be a non-empty string` };
      }
      if (value.length > 50) return { valid: false, error: `${fieldPath} timezone name too long` };
      const validPrefixes = ["Africa/","America/","Antarctica/","Asia/","Atlantic/","Australia/","Europe/","Indian/","Pacific/","Etc/"];
      const ok = value === "UTC" || value === "GMT" || validPrefixes.some(p => value.startsWith(p));
      if (!ok || value.startsWith("/") || value.endsWith("/") || value.includes(" ")) {
        return { valid: false, error: `${fieldPath} invalid IANA timezone format: ${value}` };
      }
      return { valid: true };
    }
    case VALIDATION_TYPES.GPIO:
      if (typeof value !== "number") return { valid: false, error: `${fieldPath} must be a number` };
      if (!VALID_GPIOS.includes(value)) return { valid: false, error: `${fieldPath} invalid GPIO pin: ${value}` };
      if (!SAFE_GPIOS.includes(value)) return { valid: false, error: `${fieldPath} GPIO ${value} is not safe to use` };
      return { valid: true };
    case VALIDATION_TYPES.RANGE_INT:
      if (typeof value !== "number") return { valid: false, error: `${fieldPath} must be a number` };
      if (value < fieldDef.min || value > fieldDef.max) return { valid: false, error: `${fieldPath} must be between ${fieldDef.min} and ${fieldDef.max}` };
      return { valid: true };
    case VALIDATION_TYPES.BOOLEAN:
      return { valid: typeof value === "boolean" };
    case VALIDATION_TYPES.ENUM_STRING:
      if (typeof value !== "string") return { valid: false, error: `${fieldPath} must be a string` };
      return { valid: fieldDef.values.includes(value) };
    default:
      return { valid: false, error: `${fieldPath} unsupported type: ${fieldDef.type}` };
  }
}

function flattenObject(obj, prefix = "") {
  const out = {};
  for (const key in obj) {
    if (Object.prototype.hasOwnProperty.call(obj, key)) {
      const val = obj[key];
      if (val && typeof val === "object" && !Array.isArray(val)) {
        Object.assign(out, flattenObject(val, `${prefix}${key}.`));
      } else {
        out[`${prefix}${key}`] = val;
      }
    }
  }
  return out;
}

function validateConfigFields(configUpdate) {
  const flat = flattenObject(configUpdate);
  const rules = {
    "device.owner":     { type: VALIDATION_TYPES.NON_EMPTY_STRING },
    "device.timezone":  { type: VALIDATION_TYPES.IANA_TIMEZONE },
    "device.maxCharacters": { type: VALIDATION_TYPES.RANGE_INT, min: 100, max: 10000 },
    "device.printerTxPin":  { type: VALIDATION_TYPES.GPIO },
    "wifi.ssid":        { type: VALIDATION_TYPES.NON_EMPTY_STRING },
    "wifi.password":    { type: VALIDATION_TYPES.STRING },
    "wifi.connect_timeout": { type: VALIDATION_TYPES.RANGE_INT, min: 1000, max: 30000 },
    "mqtt.enabled":     { type: VALIDATION_TYPES.BOOLEAN },
    "mqtt.server":      { type: VALIDATION_TYPES.STRING },
    "mqtt.port":        { type: VALIDATION_TYPES.RANGE_INT, min: 1, max: 65535 },
    "mqtt.username":    { type: VALIDATION_TYPES.STRING },
    "mqtt.password":    { type: VALIDATION_TYPES.STRING },
    "unbiddenInk.enabled": { type: VALIDATION_TYPES.BOOLEAN },
    "unbiddenInk.startHour": { type: VALIDATION_TYPES.RANGE_INT, min: 0, max: 24 },
    "unbiddenInk.endHour":   { type: VALIDATION_TYPES.RANGE_INT, min: 0, max: 24 },
    "unbiddenInk.frequencyMinutes": { type: VALIDATION_TYPES.RANGE_INT, min: 1, max: 1440 },
  };
  for (const k in flat) {
    if (!rules[k]) continue;
    const r = validateField(k, flat[k], rules[k]);
    if (!r.valid) return r;
  }
  return { valid: true };
}

function logProcessedFields(configUpdate) {
  const flat = flattenObject(configUpdate);
  const keys = Object.keys(flat);
  console.log(`✅ Processed ${keys.length} fields:`);
  keys.forEach(k => console.log(`   • ${k}: ${JSON.stringify(flat[k])}`));
}

// =============================
// Data loading
// =============================

function loadMockData() {
  const read = (p) => JSON.parse(fs.readFileSync(path.join(__dirname, "data", p), "utf8"));
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

let {
  mockConfig,
  mockConfigAPMode,
  mockConfigNoLEDs,
  mockDiagnostics,
  mockPrinterDiscovery,
  mockNvsDump,
  mockWifiScan,
  mockMemos,
  mockRoutes,
} = loadMockData();

// =============================
// Handlers
// =============================

function isAPMode() { return currentMode === "ap-mode"; }

function handleConnectivityProbes(pathname) {
  if (pathname === "/hotspot-detect.html" || pathname === "/generate_204" || pathname === "/connectivity-check.html" || pathname === "/ncsi.txt") {
    return true;
  }
  return false;
}

function serveConnectivity(req, res, pathname) {
  if (isAPMode()) {
    res.writeHead(302, { Location: "/setup.html", "Access-Control-Allow-Origin": "*" });
    res.end("Redirecting to setup page...");
    return;
  }
  if (pathname === "/generate_204") {
    res.writeHead(204, { "Access-Control-Allow-Origin": "*" });
    res.end();
  } else if (pathname === "/ncsi.txt") {
    serveText(res, 200, "text/plain", "Microsoft NCSI");
  } else {
    serveText(res, 200, "text/html", "<html><body>OK</body></html>");
  }
}

function handleSSE(req, res) {
  res.writeHead(200, {
    "Content-Type": "text/event-stream",
    "Cache-Control": "no-cache",
    Connection: "keep-alive",
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Headers": "Cache-Control",
  });

  const data = { ...mockPrinterDiscovery };
  res.write(`event: printer-update\ndata: ${JSON.stringify(data)}\n\n`);
  const interval = setInterval(() => {
    if (res.destroyed) return clearInterval(interval);
    data.discovered_printers[0].last_power_on = new Date().toISOString();
    res.write(`event: printer-update\ndata: ${JSON.stringify(data)}\n\n`);
  }, 30000);
  req.on("close", () => clearInterval(interval));
}

function handleAPStatic(pathname, res) {
  // Root serves setup.html
  if (pathname === "/") {
    const p = path.join(__dirname, "..", "data", "setup.html");
    return serveFileOr404(res, p, { apMode: true });
  }
  // Allowlist; otherwise redirect handled by caller
  const allowed = ["/setup.html","/css/","/js/","/images/","/fonts/","/site.webmanifest","/favicon.ico","/favicon.svg","/favicon-96x96.png","/apple-touch-icon.png"];
  const ok = allowed.some((p) => pathname === p || pathname.startsWith(p));
  if (!ok) {
    res.writeHead(302, { Location: "/setup.html" });
    res.end();
    return;
  }
  const reqPath = pathname.substring(1);
  const filePath = path.join(__dirname, "..", "data", reqPath);
  serveFileOr404(res, filePath, { apMode: true });
}

function handleSTAStatic(pathname, res) {
  if (pathname === "/") {
    const p = path.join(__dirname, "..", "data", "index.html");
    // console.debug('STA root static path:', p);
    return serveFileOr404(res, p);
  }
  if (pathname === "/setup.html") {
    return sendNotFound(res);
  }
  const reqPath = pathname.substring(1) || "index.html";
  const filePath = path.join(__dirname, "..", "data", reqPath);
  // console.debug('STA static path:', filePath);
  serveFileOr404(res, filePath);
}

// =============================
// Router
// =============================

function createRequestHandler() {
  return (req, res) => {
    const { pathname } = new URL(req.url, `http://${req.headers.host}`);

    // Preflight
    if (req.method === "OPTIONS") {
      res.writeHead(200, {
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Methods": "GET, POST, PUT, DELETE, OPTIONS",
        "Access-Control-Allow-Headers": "Content-Type, Authorization",
      });
      res.end();
      return;
    }

    // SSE
    if (pathname === "/mqtt-printers") {
      return handleSSE(req, res);
    }

    // Connectivity probes
    if (handleConnectivityProbes(pathname)) {
      return serveConnectivity(req, res, pathname);
    }

    // Favicon assets
    if (pathname === "/favicon.ico" || pathname === "/favicon-96x96.png" || pathname === "/apple-touch-icon.png") {
      const filePath = path.join(__dirname, "..", "data", pathname);
      fs.readFile(filePath, (err, data) => {
        if (err) return sendNotFound(res);
        const contentType = pathname.endsWith(".ico") ? "image/x-icon" : "image/png";
        res.writeHead(200, { "Content-Type": contentType, "Cache-Control": "public, max-age=604800", "Access-Control-Allow-Origin": "*" });
        res.end(data);
      });
      return;
    }

    // API
    if (pathname.startsWith("/api/")) {
      const ctx = {
        mockConfig,
        mockConfigAPMode,
        mockConfigNoLEDs,
        mockDiagnostics,
        mockNvsDump,
        mockWifiScan,
        mockMemos,
        validateConfigFields,
        logProcessedFields,
        startTime,
        currentMode,
      };
      if (handleAPI(req, res, pathname, ctx)) return;
      return;
    }

    // Debug
    if (pathname === "/debug/filesystem") {
      const fsPath = path.join(__dirname, "data", "mock-filesystem.txt");
      fs.readFile(fsPath, "utf8", (err, data) => {
        if (err) return sendNotFound(res);
        res.writeHead(200, { "Content-Type": "text/plain", "Access-Control-Allow-Origin": "*" });
        res.end(data);
      });
      return;
    }

    // Static by mode (avoid ASI issues by using blocks)
    if (isAPMode()) {
      handleAPStatic(pathname, res);
      return;
    }
    handleSTAStatic(pathname, res);
  };
}

// Start server
const server = http.createServer(createRequestHandler());
server.listen(PORT, () => {
  printStartupHelp();
  setupKeyboardShortcuts(server);
});

function printStartupHelp() {
  console.log("===================================================");
  console.log(`🚀 Mock server started at: http://localhost:${PORT}`);
  console.log(`📦 Mode: ${currentMode}`);
  console.log("");
  console.log("Usage:");
  console.log("  node mock-api.js             # STA-like mode");
  console.log("  node mock-api.js --ap-mode   # AP captive-portal mode");
  console.log("  node mock-api.js --no-leds   # STA mode with LEDs disabled");
  console.log("  node mock-api.js --disable-mqtt # STA mode with MQTT disabled");
  console.log("");
  console.log("Keyboard shortcuts:");
  console.log("  r  Reload JSON data files (config, diagnostics, routes, etc.)");
  console.log("  q  Quit (graceful)");
  console.log("  x  Quit (graceful)");
  console.log("  Ctrl+C  Quit (graceful)");
  console.log("===================================================");
}

function setupKeyboardShortcuts(serverInstance) {
  if (!process.stdin.isTTY) return;
  process.stdin.setEncoding("utf8");
  process.stdin.resume();

  process.stdin.on("data", (chunk) => {
    const input = (chunk || "").toString().trim().toLowerCase();
    if (!input) return;
    if (input === "r") {
      try {
        const reloaded = loadMockData();
        mockConfig = reloaded.mockConfig;
        mockConfigAPMode = reloaded.mockConfigAPMode;
        mockConfigNoLEDs = reloaded.mockConfigNoLEDs;
        mockDiagnostics = reloaded.mockDiagnostics;
        mockPrinterDiscovery = reloaded.mockPrinterDiscovery;
        mockNvsDump = reloaded.mockNvsDump;
        mockWifiScan = reloaded.mockWifiScan;
        mockMemos = reloaded.mockMemos;
        mockRoutes = reloaded.mockRoutes;
        console.log("✅ Reloaded JSON data files\n");
      } catch (e) {
        console.error("❌ Failed to reload JSON data:", e?.message || e);
      }
      return;
    }
    if (input === "q" || input === "x") {
      gracefulShutdown(serverInstance);
      return;
    }
  });

  process.on("SIGINT", () => gracefulShutdown(serverInstance));
}

function gracefulShutdown(serverInstance) {
  console.log("\nShutting down mock server...");
  const timeout = setTimeout(() => process.exit(0), 1000);
  serverInstance.close(() => {
    clearTimeout(timeout);
    console.log("✅ Server stopped. Bye!");
    process.exit(0);
  });
}
