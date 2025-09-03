#!/usr/bin/env node

/**
 * Mock API Server for Scribe Evolution Local Development
 * Uses only Node.js built-ins - no external dependencies needed!
 *
 * Usage: node mock-api.js
 * Then access: http://localhost:3001/
 */

const http = require("http");
const fs = require("fs");
const path = require("path");

const PORT = 3001;
const startTime = Date.now();

// Check for command line arguments
const args = process.argv.slice(2);
const currentMode = args.includes("--ap-mode")
  ? "ap-mode"
  : args.includes("--no-leds")
    ? "no-leds"
    : args.includes("--disable-mqtt")
      ? "disable-mqtt"
      : "normal";

// ANSI color codes for console output
const colors = {
  red: "\x1b[31m",
  green: "\x1b[32m",
  yellow: "\x1b[33m",
  blue: "\x1b[34m",
  reset: "\x1b[0m",
};

// Helper functions for colored console output
function logError(...args) {
  console.error(colors.red + args.join(" ") + colors.reset);
}

function logSuccess(...args) {
  console.log(colors.green + args.join(" ") + colors.reset);
}

// =============================
// Lightweight Router Helpers
// =============================

function isAPMode() {
  return currentMode === "ap-mode";
}

function serveText(res, status, contentType, body) {
  res.writeHead(status, {
    "Content-Type": contentType,
    "Access-Control-Allow-Origin": "*",
  });
  res.end(body);
}

function serveFileOr404(res, filePath) {
  if (fs.existsSync(filePath) || fs.existsSync(filePath + ".gz")) {
    return serveFile(res, filePath);
  }
  return sendNotFound(res);
}

function handleConnectivityProbes(req, res, pathname) {
  // Captive portal probes
  if (
    pathname === "/hotspot-detect.html" ||
    pathname === "/generate_204" ||
    pathname === "/connectivity-check.html" ||
    pathname === "/ncsi.txt"
  ) {
    if (isAPMode()) {
      // In AP mode redirect to setup
      res.writeHead(302, { Location: "/setup.html", "Access-Control-Allow-Origin": "*" });
      res.end("Redirecting to setup page...");
      return true;
    }
    // STA mode responses
    if (pathname === "/generate_204") {
      res.writeHead(204, { "Access-Control-Allow-Origin": "*" });
      res.end();
      return true;
    }
    if (pathname === "/ncsi.txt") {
      serveText(res, 200, "text/plain", "Microsoft NCSI");
      return true;
    }
    serveText(res, 200, "text/html", "<html><body>OK</body></html>");
    return true;
  }
  return false;
}

function handleAPStatic(req, res, pathname) {
  // Allow-list check to decide redirect vs serve
  const allowed = [
    "/setup.html",
    "/css/",
    "/js/",
    "/images/",
    "/fonts/",
    "/site.webmanifest",
    "/favicon.ico",
    "/favicon.svg",
    "/favicon-96x96.png",
    "/apple-touch-icon.png",
  ];

  const isAllowed = allowed.some((p) => pathname === p || pathname.startsWith(p));

  if (!isAllowed && pathname !== "/") {
    res.writeHead(302, { Location: "/setup.html" });
    res.end();
    return;
  }

  if (pathname === "/") {
    const setupPath = path.join(__dirname, "..", "data", "setup.html");
    if (fs.existsSync(setupPath)) {
      fs.readFile(setupPath, (err, data) => {
        if (err) return sendNotFound(res);
        res.writeHead(200, {
          "Content-Type": "text/html",
          "Cache-Control": "no-cache",
          "Access-Control-Allow-Origin": "*",
        });
        res.end(data);
      });
      return;
    }
    return sendNotFound(res);
  }

  // Serve the file directly
  const requestPath = pathname.substring(1);
  const filePath = path.join(__dirname, "..", "data", requestPath);
  serveFileOr404(res, filePath);
}

function handleSTAStatic(req, res, pathname) {
  // Root defaults to index.html
  if (pathname === "/") {
    const indexPath = path.join(__dirname, "..", "data", "index.html");
    return serveFileOr404(res, indexPath);
  }
  // Setup is not available in STA mode
  if (pathname === "/setup.html") {
    return sendNotFound(res);
  }
  // Direct static serve
  const requestPath = pathname.substring(1) || "index.html";
  const filePath = path.join(__dirname, "..", "data", requestPath);
  serveFileOr404(res, filePath);
}

// Data-driven validation system - mirrors ESP32 CONFIG_FIELDS array
const VALIDATION_TYPES = {
  STRING: "string",
  NON_EMPTY_STRING: "non_empty_string",
  IANA_TIMEZONE: "iana_timezone",
  GPIO: "gpio",
  RANGE_INT: "range_int",
  BOOLEAN: "boolean",
  ENUM_STRING: "enum_string",
};

const VALID_BUTTON_ACTIONS = [
  "JOKE",
  "RIDDLE",
  "QUOTE",
  "QUIZ",
  "NEWS",
  "CHARACTER_TEST",
  "UNBIDDEN_INK",
  "MEMO1",
  "MEMO2",
  "MEMO3",
  "MEMO4",
  "",
];
const VALID_LED_EFFECTS = [
  "chase_single",
  "chase_multi",
  "rainbow",
  "twinkle",
  "pulse",
  "matrix",
  "none",
];

// ESP32-C3 GPIO validation - mirrors config.h ESP32C3_GPIO_MAP
const VALID_GPIOS = [-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 21];
const SAFE_GPIOS = [-1, 2, 4, 5, 6, 7, 10, 20, 21]; // Only GPIO_TYPE_SAFE pins from config.h

const CONFIG_FIELDS = {
  // Device configuration
  "device.owner": { type: VALIDATION_TYPES.NON_EMPTY_STRING },
  "device.timezone": { type: VALIDATION_TYPES.IANA_TIMEZONE },
  "device.maxCharacters": {
    type: VALIDATION_TYPES.RANGE_INT,
    min: 100,
    max: 10000,
  },
  "device.printerTxPin": { type: VALIDATION_TYPES.GPIO },

  // WiFi configuration
  "wifi.ssid": { type: VALIDATION_TYPES.NON_EMPTY_STRING },
  "wifi.password": { type: VALIDATION_TYPES.STRING },
  "wifi.connect_timeout": {
    type: VALIDATION_TYPES.RANGE_INT,
    min: 1000,
    max: 30000,
  },

  // MQTT configuration
  "mqtt.enabled": { type: VALIDATION_TYPES.BOOLEAN },
  "mqtt.server": { type: VALIDATION_TYPES.STRING },
  "mqtt.port": { type: VALIDATION_TYPES.RANGE_INT, min: 1, max: 65535 },
  "mqtt.username": { type: VALIDATION_TYPES.STRING },
  "mqtt.password": { type: VALIDATION_TYPES.STRING },

  // Unbidden Ink configuration
  "unbiddenInk.enabled": { type: VALIDATION_TYPES.BOOLEAN },
  "unbiddenInk.startHour": {
    type: VALIDATION_TYPES.RANGE_INT,
    min: 0,
    max: 24,
  },
  "unbiddenInk.endHour": { type: VALIDATION_TYPES.RANGE_INT, min: 0, max: 24 },
  "unbiddenInk.frequencyMinutes": {
    type: VALIDATION_TYPES.RANGE_INT,
    min: 15,
    max: 480,
  },
  "unbiddenInk.prompt": { type: VALIDATION_TYPES.NON_EMPTY_STRING },
  "unbiddenInk.chatgptApiToken": { type: VALIDATION_TYPES.STRING },

  // Button configuration
  "buttons.button1.gpio": { type: VALIDATION_TYPES.GPIO },
  "buttons.button2.gpio": { type: VALIDATION_TYPES.GPIO },
  "buttons.button3.gpio": { type: VALIDATION_TYPES.GPIO },
  "buttons.button4.gpio": { type: VALIDATION_TYPES.GPIO },
  "buttons.button1.shortAction": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_BUTTON_ACTIONS,
  },
  "buttons.button1.longAction": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_BUTTON_ACTIONS,
  },
  "buttons.button2.shortAction": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_BUTTON_ACTIONS,
  },
  "buttons.button2.longAction": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_BUTTON_ACTIONS,
  },
  "buttons.button3.shortAction": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_BUTTON_ACTIONS,
  },
  "buttons.button3.longAction": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_BUTTON_ACTIONS,
  },
  "buttons.button4.shortAction": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_BUTTON_ACTIONS,
  },
  "buttons.button4.longAction": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_BUTTON_ACTIONS,
  },
  "buttons.button1.shortLedEffect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
  "buttons.button1.longLedEffect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
  "buttons.button2.shortLedEffect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
  "buttons.button2.longLedEffect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
  "buttons.button3.shortLedEffect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
  "buttons.button3.longLedEffect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
  "buttons.button4.shortLedEffect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
  "buttons.button4.longLedEffect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
  "buttons.button1.shortMqttTopic": { type: VALIDATION_TYPES.STRING },
  "buttons.button1.longMqttTopic": { type: VALIDATION_TYPES.STRING },
  "buttons.button2.shortMqttTopic": { type: VALIDATION_TYPES.STRING },
  "buttons.button2.longMqttTopic": { type: VALIDATION_TYPES.STRING },
  "buttons.button3.shortMqttTopic": { type: VALIDATION_TYPES.STRING },
  "buttons.button3.longMqttTopic": { type: VALIDATION_TYPES.STRING },
  "buttons.button4.shortMqttTopic": { type: VALIDATION_TYPES.STRING },
  "buttons.button4.longMqttTopic": { type: VALIDATION_TYPES.STRING },

  // LED configuration
  "leds.enabled": { type: VALIDATION_TYPES.BOOLEAN },
  "leds.pin": { type: VALIDATION_TYPES.GPIO },
  "leds.count": { type: VALIDATION_TYPES.RANGE_INT, min: 1, max: 300 },
  "leds.brightness": { type: VALIDATION_TYPES.RANGE_INT, min: 0, max: 255 },
  "leds.refreshRate": { type: VALIDATION_TYPES.RANGE_INT, min: 10, max: 120 },
  "leds.speed": { type: VALIDATION_TYPES.RANGE_INT, min: 1, max: 100 },
  "leds.intensity": { type: VALIDATION_TYPES.RANGE_INT, min: 1, max: 100 },
  "leds.cycles": { type: VALIDATION_TYPES.RANGE_INT, min: 1, max: 10 },
  "leds.effect": {
    type: VALIDATION_TYPES.ENUM_STRING,
    values: VALID_LED_EFFECTS,
  },
};

function validateField(fieldPath, value, fieldDef) {
  switch (fieldDef.type) {
    case VALIDATION_TYPES.STRING:
      return { valid: typeof value === "string" };

    case VALIDATION_TYPES.NON_EMPTY_STRING:
      if (typeof value !== "string")
        return { valid: false, error: `${fieldPath} must be a string` };
      if (value.length === 0)
        return { valid: false, error: `${fieldPath} cannot be empty` };
      return { valid: true };

    case VALIDATION_TYPES.IANA_TIMEZONE:
      if (typeof value !== "string")
        return { valid: false, error: `${fieldPath} must be a string` };
      if (value.length === 0)
        return { valid: false, error: `${fieldPath} cannot be empty` };

      // Basic IANA timezone format validation (mirrors backend logic)
      if (value.length > 50)
        return { valid: false, error: `${fieldPath} timezone name too long` };
      if (
        value.indexOf("/") === -1 &&
        value !== "UTC" &&
        value !== "GMT" &&
        !value.startsWith("Etc/")
      ) {
        return {
          valid: false,
          error: `${fieldPath} invalid IANA timezone format: ${value} (expected format: Area/Location, e.g., America/New_York, Europe/London)`,
        };
      }
      if (value.startsWith("/") || value.endsWith("/")) {
        return {
          valid: false,
          error: `${fieldPath} invalid IANA timezone format: ${value} (cannot start or end with slash)`,
        };
      }
      if (value.indexOf(" ") !== -1) {
        return {
          valid: false,
          error: `${fieldPath} invalid IANA timezone format: ${value} (spaces not allowed, use underscores)`,
        };
      }

      // Check for valid IANA timezone prefixes
      const validPrefixes = [
        "Africa/",
        "America/",
        "Antarctica/",
        "Asia/",
        "Atlantic/",
        "Australia/",
        "Europe/",
        "Indian/",
        "Pacific/",
        "Etc/",
      ];
      const hasValidPrefix =
        validPrefixes.some((prefix) => value.startsWith(prefix)) ||
        value === "UTC" ||
        value === "GMT";
      if (!hasValidPrefix) {
        return {
          valid: false,
          error: `${fieldPath} invalid IANA timezone format: ${value} (expected format: Area/Location, e.g., America/New_York, Europe/London)`,
        };
      }

      return { valid: true };

    case VALIDATION_TYPES.GPIO:
      if (typeof value !== "number")
        return { valid: false, error: `${fieldPath} must be a number` };
      if (!VALID_GPIOS.includes(value))
        return {
          valid: false,
          error: `${fieldPath} invalid GPIO pin: ${value}`,
        };
      if (!SAFE_GPIOS.includes(value))
        return {
          valid: false,
          error: `${fieldPath} GPIO ${value} is not safe to use`,
        };
      return { valid: true };

    case VALIDATION_TYPES.RANGE_INT:
      if (typeof value !== "number")
        return { valid: false, error: `${fieldPath} must be a number` };
      if (value < fieldDef.min || value > fieldDef.max) {
        return {
          valid: false,
          error: `${fieldPath} must be between ${fieldDef.min} and ${fieldDef.max}`,
        };
      }
      return { valid: true };

    case VALIDATION_TYPES.BOOLEAN:
      if (typeof value !== "boolean")
        return { valid: false, error: `${fieldPath} must be a boolean` };
      return { valid: true };

    case VALIDATION_TYPES.ENUM_STRING:
      if (typeof value !== "string")
        return { valid: false, error: `${fieldPath} must be a string` };
      if (!fieldDef.values.includes(value)) {
        return { valid: false, error: `${fieldPath} invalid value: ${value}` };
      }
      return { valid: true };

    default:
      return {
        valid: false,
        error: `${fieldPath} unsupported validation type: ${fieldDef.type}`,
      };
  }
}

function flattenObject(obj, prefix = "") {
  const flattened = {};
  for (const key in obj) {
    if (
      typeof obj[key] === "object" &&
      obj[key] !== null &&
      !Array.isArray(obj[key])
    ) {
      Object.assign(flattened, flattenObject(obj[key], prefix + key + "."));
    } else {
      flattened[prefix + key] = obj[key];
    }
  }
  return flattened;
}

function validateConfigFields(configUpdate) {
  const flatConfig = flattenObject(configUpdate);

  for (const [fieldPath, value] of Object.entries(flatConfig)) {
    const fieldDef = CONFIG_FIELDS[fieldPath];
    if (!fieldDef) {
      return {
        valid: false,
        error: `Unknown configuration field: ${fieldPath}`,
      };
    }

    const validation = validateField(fieldPath, value, fieldDef);
    if (!validation.valid) {
      return { valid: false, error: validation.error };
    }
  }

  return { valid: true };
}

function logProcessedFields(configUpdate) {
  const flatConfig = flattenObject(configUpdate);
  const processedFields = Object.keys(flatConfig);

  console.log(`✅ Processed ${processedFields.length} fields:`);
  processedFields.forEach((field) => {
    console.log(`   • ${field}: ${JSON.stringify(flatConfig[field])}`);
  });
}

// Load mock data from JSON files
function loadMockData() {
  try {
    const mockConfig = JSON.parse(
      fs.readFileSync(path.join(__dirname, "data/mock-config.json"), "utf8"),
    );
    const mockConfigAPMode = JSON.parse(
      fs.readFileSync(
        path.join(__dirname, "data/mock-config-ap-mode.json"),
        "utf8",
      ),
    );
    const mockConfigNoLEDs = JSON.parse(
      fs.readFileSync(
        path.join(__dirname, "data/mock-config-no-leds.json"),
        "utf8",
      ),
    );
    const mockDiagnostics = JSON.parse(
      fs.readFileSync(
        path.join(__dirname, "data/mock-diagnostics.json"),
        "utf8",
      ),
    );
    const mockPrinterDiscovery = JSON.parse(
      fs.readFileSync(
        path.join(__dirname, "data/mock-printer-discovery.json"),
        "utf8",
      ),
    );
    const mockNvsDump = JSON.parse(
      fs.readFileSync(path.join(__dirname, "data/mock-nvs-dump.json"), "utf8"),
    );
    const mockWifiScan = JSON.parse(
      fs.readFileSync(path.join(__dirname, "data/mock-wifi-scan.json"), "utf8"),
    );
    const mockMemos = JSON.parse(
      fs.readFileSync(path.join(__dirname, "data/mock-memos.json"), "utf8"),
    );
    const mockRoutes = JSON.parse(
      fs.readFileSync(path.join(__dirname, "data/mock-routes.json"), "utf8"),
    );

    return {
      mockConfig,
      mockConfigAPMode,
      mockConfigNoLEDs,
      mockDiagnostics,
      mockPrinterDiscovery,
      mockNvsDump,
      mockWifiScan,
      mockMemos,
      mockRoutes,
    };
  } catch (error) {
    logError("Error loading mock data files:", error.message);
    process.exit(1);
  }
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

// MIME types
const mimeTypes = {
  ".html": "text/html",
  ".css": "text/css",
  ".js": "application/javascript",
  ".json": "application/json",
  ".png": "image/png",
  ".svg": "image/svg+xml",
  ".ico": "image/x-icon",
  ".txt": "text/plain",
  ".webmanifest": "application/manifest+json",
  ".woff2": "font/woff2",
};

function formatUptime(ms) {
  const seconds = Math.floor(ms / 1000);
  const minutes = Math.floor(seconds / 60);
  const hours = Math.floor(minutes / 60);

  if (hours > 0) {
    return `${hours}h ${minutes % 60}m ${seconds % 60}s`;
  } else if (minutes > 0) {
    return `${minutes}m ${seconds % 60}s`;
  } else {
    return `${seconds}s`;
  }
}

function expandPlaceholders(content) {
  let expanded = content;

  // Date placeholders
  const now = new Date();
  expanded = expanded.replace(
    /\[date\]/g,
    now
      .toLocaleDateString("en-GB", {
        day: "2-digit",
        month: "short",
        year: "2-digit",
      })
      .replace(/ /g, ""),
  );
  expanded = expanded.replace(
    /\[time\]/g,
    now.toLocaleTimeString("en-US", {
      hour: "2-digit",
      minute: "2-digit",
      hour12: false,
    }),
  );
  expanded = expanded.replace(
    /\[weekday\]/g,
    now.toLocaleDateString("en-US", { weekday: "long" }),
  );

  // Random placeholders
  expanded = expanded.replace(
    /\[coin\]/g,
    Math.random() > 0.5 ? "Heads" : "Tails",
  );
  expanded = expanded.replace(
    /\[dice:(\d+)\]/g,
    (match, sides) => Math.floor(Math.random() * parseInt(sides)) + 1,
  );
  expanded = expanded.replace(/\[dice\]/g, Math.floor(Math.random() * 6) + 1); // default 6-sided

  // Pick random option
  expanded = expanded.replace(/\[pick:([^\]]+)\]/g, (match, options) => {
    const choices = options.split("|");
    return choices[Math.floor(Math.random() * choices.length)];
  });

  // Device info
  expanded = expanded.replace(
    /\[uptime\]/g,
    `${Math.floor(Math.random() * 12)}h${Math.floor(Math.random() * 60)}m`,
  );
  expanded = expanded.replace(/\[ip\]/g, "192.168.1.100");
  expanded = expanded.replace(/\[mdns\]/g, "scribe.local");

  return expanded;
}

function sendJSON(res, data, statusCode = 200) {
  res.writeHead(statusCode, {
    "Content-Type": "application/json",
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "GET, POST, PUT, DELETE, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type, Authorization",
  });
  res.end(JSON.stringify(data, null, 2));
}

function sendNotFound(res) {
  const baseDir = path.join(__dirname, "..", "data");
  const gz404 = path.join(baseDir, "404.html.gz");
  const html404 = path.join(baseDir, "404.html");
  if (fs.existsSync(gz404)) {
    fs.readFile(gz404, (err, data) => {
      if (err) {
        res.writeHead(404, {
          "Content-Type": "text/html",
          "Access-Control-Allow-Origin": "*",
        });
        return res.end("<h1>404 Not Found</h1>");
      }
      res.writeHead(404, {
        "Content-Type": "text/html",
        "Content-Encoding": "gzip",
        "Access-Control-Allow-Origin": "*",
        "Cache-Control": "no-cache",
      });
      return res.end(data);
    });
  } else if (fs.existsSync(html404)) {
    fs.readFile(html404, (err, data) => {
      if (err) {
        res.writeHead(404, {
          "Content-Type": "text/html",
          "Access-Control-Allow-Origin": "*",
        });
        return res.end("<h1>404 Not Found</h1>");
      }
      res.writeHead(404, {
        "Content-Type": "text/html",
        "Access-Control-Allow-Origin": "*",
        "Cache-Control": "no-cache",
      });
      return res.end(data);
    });
  } else {
    res.writeHead(404, {
      "Content-Type": "text/html",
      "Access-Control-Allow-Origin": "*",
    });
    return res.end("<h1>404 Not Found</h1>");
  }
}

function serveFile(res, filePath, statusCode = 200) {
  const ext = path.extname(filePath).toLowerCase();
  const contentType = mimeTypes[ext] || "application/octet-stream";

  // Determine which files should be served compressed
  const compressibleTypes = [
    ".html",
    ".css",
    ".js",
    ".json",
    ".svg",
    ".txt",
    ".md",
  ];
  // Mirror device behavior:
  // - STA mode: prefer gzip for text assets
  // - AP mode: serve uncompressed for HTML/CSS/JS to support captive portal mini-browsers
  const isAPMode = currentMode === "ap-mode";
  const isTextType = compressibleTypes.includes(ext);
  const isCoreWebAsset = ext === ".html" || ext === ".css" || ext === ".js";
  const shouldCompress = isTextType && !(isAPMode && isCoreWebAsset);

  if (shouldCompress) {
    // Serve compressed version for text files
    const gzipPath = filePath + ".gz";
    fs.readFile(gzipPath, (err, data) => {
      if (err) {
        return sendNotFound(res);
      }

      res.writeHead(statusCode, {
        "Content-Type": contentType,
        "Content-Encoding": "gzip",
        "Access-Control-Allow-Origin": "*",
        "Cache-Control": "public, max-age=31536000",
      });
      res.end(data);
    });
  } else {
    // Serve uncompressed version for binary files (png, ico, webmanifest)
    fs.readFile(filePath, (err, data) => {
      if (err) {
        return sendNotFound(res);
      }

      res.writeHead(statusCode, {
        "Content-Type": contentType,
        "Access-Control-Allow-Origin": "*",
        "Cache-Control": "public, max-age=31536000",
      });
      res.end(data);
    });
  }
}

// DRY function for server startup messages
function logServerStartup() {
  console.log("===================================================");
  console.log(`🚀 Mock server started at: http://localhost:${PORT}`);

  // Show current mode with all status details
  console.log("📋 Configuration Status:");

  if (currentMode === "ap-mode") {
    console.log("  🚀 Mode: AP Setup");
    console.log("  📶 Network: AP Mode (WiFi hotspot)");
    console.log("  💡 LEDs: Enabled");
    console.log("  📡 MQTT: Enabled");
  } else if (currentMode === "no-leds") {
    console.log("  ⚙️  Mode: No LEDs");
    console.log("  📶 Network: STA Mode (connect to WiFi)");
    console.log("  💡 LEDs: Disabled");
    console.log("  📡 MQTT: Enabled");
  } else if (currentMode === "disable-mqtt") {
    console.log("  📡 Mode: MQTT Disabled");
    console.log("  📶 Network: STA Mode (connect to WiFi)");
    console.log("  💡 LEDs: Enabled");
    console.log("  📡 MQTT: Disabled");
  } else {
    console.log("  ⚙️  Mode: Normal");
    console.log("  📶 Network: STA Mode (connect to WiFi)");
    console.log("  💡 LEDs: Enabled");
    console.log("  📡 MQTT: Enabled");
  }

  console.log("Commands:");
  console.log('"r" + Enter to restart and pick up HTML/CSS/JS changes.');
  console.log('"d" + Enter to reload JSON data files');
  console.log('"x" + Enter to stop gracefully (or CTRL-C');
  console.log("Usage: node mock-api.js [--ap-mode|--no-leds|--disable-mqtt]");
}

// Create server request handler (DRY - used for initial server and restarts)
function createRequestHandler() {
  return (req, res) => {
    const reqUrl = new URL(req.url, `http://${req.headers.host}`);
    const pathname = reqUrl.pathname;
    const urlQuery = Object.fromEntries(reqUrl.searchParams);

    console.log(`${new Date().toISOString()} ${req.method} ${pathname}`);

    // Handle CORS preflight
    if (req.method === "OPTIONS") {
      res.writeHead(200, {
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Methods": "GET, POST, PUT, DELETE, OPTIONS",
        "Access-Control-Allow-Headers": "Content-Type, Authorization",
      });
      res.end();
      return;
    }

    // Handle SSE endpoints that don't have /api/ prefix
    if (pathname === "/mqtt-printers") {
      // Server-Sent Events for general events
      res.writeHead(200, {
        "Content-Type": "text/event-stream",
        "Cache-Control": "no-cache",
        Connection: "keep-alive",
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Headers": "Cache-Control",
      });

      console.log("🔌 SSE: MQTT printers SSE connection established");

      // Send initial printer discovery data with correct event type
      const printerDiscoveryData = { ...mockPrinterDiscovery };

      res.write(
        `event: printer-update\ndata: ${JSON.stringify(printerDiscoveryData)}\n\n`,
      );

      const interval = setInterval(() => {
        if (res.destroyed) {
          clearInterval(interval);
          return;
        }
        // Send updated printer discovery data periodically with correct event type
        printerDiscoveryData.discovered_printers[0].last_power_on =
          new Date().toISOString();
        res.write(
          `event: printer-update\ndata: ${JSON.stringify(printerDiscoveryData)}\n\n`,
        );
      }, 30000);

      req.on("close", () => {
        console.log("🔌 SSE: MQTT printers SSE connection closed");
        clearInterval(interval);
      });
      return;
    }

    // Captive portal detection URLs - handle in AP mode
    if (
      pathname === "/hotspot-detect.html" ||
      pathname === "/generate_204" ||
      pathname === "/connectivity-check.html" ||
      pathname === "/ncsi.txt"
    ) {
      if (currentMode === "ap-mode") {
        // In AP mode, redirect to setup.html
        res.writeHead(302, {
          Location: "/setup.html",
          "Access-Control-Allow-Origin": "*",
        });
        res.end("Redirecting to setup page...");
        console.log(`🌐 Captive portal detection: ${pathname} -> /setup.html`);
      } else {
        // In normal mode, return appropriate responses
        if (pathname === "/generate_204") {
          // Android/Chrome expects 204 No Content for internet connectivity
          res.writeHead(204, {
            "Access-Control-Allow-Origin": "*",
          });
          res.end();
        } else if (pathname === "/ncsi.txt") {
          // Windows NCSI check expects specific plain text
          res.writeHead(200, {
            "Content-Type": "text/plain",
            "Access-Control-Allow-Origin": "*",
          });
          res.end("Microsoft NCSI");
        } else {
          // Other platforms expect simple HTML responses
          res.writeHead(200, {
            "Content-Type": "text/html",
            "Access-Control-Allow-Origin": "*",
          });
          res.end("<html><body>OK</body></html>");
        }
        console.log(`🌐 Captive portal detection: ${pathname} -> OK`);
      }
      return;
    }

    // Explicit favicon/icon handling (mirrors ESP32 serveStatic with setGzip(false))
    if (
      pathname === "/favicon.ico" ||
      pathname === "/favicon-96x96.png" ||
      pathname === "/apple-touch-icon.png"
    ) {
      const filePath = path.join(__dirname, "..", "data", pathname);

      // Set appropriate MIME type and cache headers (7 days like ESP32)
      let contentType = "image/png";
      if (pathname.endsWith(".ico")) contentType = "image/x-icon";

      fs.readFile(filePath, (err, data) => {
        if (err) {
          res.writeHead(404, {
            "Content-Type": "text/plain",
            "Access-Control-Allow-Origin": "*",
          });
          res.end("Favicon not found");
          return;
        }

        res.writeHead(200, {
          "Content-Type": contentType,
          "Cache-Control": "public, max-age=604800", // 7 days (mirrors ESP32)
          "Access-Control-Allow-Origin": "*",
        });
        res.end(data);
      });
      return;
    }

    // API Routes
    if (pathname.startsWith("/api/")) {
      if (pathname === "/api/test-wifi" && req.method === "POST") {
        console.log("📶 WiFi test requested (mock)");
        let body = "";
        req.on("data", (chunk) => (body += chunk));
        req.on("end", () => {
          try {
            const { ssid, password } = JSON.parse(body || "{}");
            if (!ssid || typeof ssid !== "string" || ssid.trim() === "") {
              res.writeHead(422, {
                "Content-Type": "application/json",
                "Access-Control-Allow-Origin": "*",
              });
              return res.end(
                JSON.stringify({ success: false, message: "Invalid payload" }),
              );
            }

            // Simple busy simulation via in-memory flag
            if (global.__wifiTestBusy) {
              res.writeHead(409, {
                "Content-Type": "application/json",
                "Access-Control-Allow-Origin": "*",
              });
              return res.end(
                JSON.stringify({
                  success: false,
                  message: "Test already running",
                }),
              );
            }
            global.__wifiTestBusy = true;

            const simulateLong = reqUrl.searchParams.get("long") === "1";
            const delayMs = simulateLong ? 6500 : 800;

            setTimeout(() => {
              global.__wifiTestBusy = false;

              const pwd = password || "";
              if (pwd.includes("timeout")) {
                res.writeHead(408, {
                  "Content-Type": "application/json",
                  "Access-Control-Allow-Origin": "*",
                });
                return res.end(
                  JSON.stringify({
                    success: false,
                    message: "Association timeout",
                  }),
                );
              }
              if (pwd.includes("noap")) {
                res.writeHead(400, {
                  "Content-Type": "application/json",
                  "Access-Control-Allow-Origin": "*",
                });
                return res.end(
                  JSON.stringify({ success: false, message: "No AP found" }),
                );
              }
              if (pwd.includes("auth")) {
                res.writeHead(400, {
                  "Content-Type": "application/json",
                  "Access-Control-Allow-Origin": "*",
                });
                return res.end(
                  JSON.stringify({
                    success: false,
                    message: "Authentication failed",
                  }),
                );
              }

              // Success path
              res.writeHead(200, {
                "Content-Type": "application/json",
                "Access-Control-Allow-Origin": "*",
              });
              return res.end(JSON.stringify({ success: true, rssi: -52 }));
            }, delayMs);
          } catch (e) {
            res.writeHead(422, {
              "Content-Type": "application/json",
              "Access-Control-Allow-Origin": "*",
            });
            return res.end(
              JSON.stringify({ success: false, message: "Invalid payload" }),
            );
          }
        });
        return; // do not fall through
      }
      if (pathname === "/api/config" && req.method === "GET") {
        // Support different config modes via query parameters or default mode
        // ?mode=ap-mode or ?mode=no-leds, or use currentMode from command line
        const mode = urlQuery.mode || currentMode;
        let configToSend = mockConfig;

        if (mode === "ap-mode") {
          console.log("🚀 Serving AP mode config");
          configToSend = mockConfigAPMode;
        } else if (mode === "no-leds") {
          console.log("💡 Serving no-LEDs config");
          configToSend = mockConfigNoLEDs;
        } else if (mode === "disable-mqtt") {
          console.log("📡 Serving MQTT disabled config");
          configToSend = {
            ...mockConfig,
            mqtt: { ...mockConfig.mqtt, enabled: false },
          };
        }
        setTimeout(() => sendJSON(res, configToSend), 200);
      } else if (pathname === "/api/setup" && req.method === "GET") {
        // Setup endpoint for AP mode - returns minimal config for initial setup
        console.log("🚀 Setup configuration GET request");
        const setupConfig = {
          device: {
            owner: "",
            timezone: "",
            wifi: {
              ssid: "",
              password: "",
            },
          },
        };
        setTimeout(() => sendJSON(res, setupConfig), 200);
      } else if (pathname === "/api/setup" && req.method === "POST") {
        // Setup endpoint for AP mode - saves initial configuration
        let body = "";
        req.on("data", (chunk) => (body += chunk));
        req.on("end", () => {
          try {
            const setupData = JSON.parse(body);
            console.log(
              "🚀 Setup configuration received:",
              JSON.stringify(setupData, null, 2),
            );

            // Simulate device restart after setup
            setTimeout(() => {
              res.writeHead(200);
              res.end();
            }, 500);
          } catch (error) {
            console.error("Error parsing setup JSON:", error.message);
            setTimeout(() => {
              sendJSON(
                res,
                {
                  error: "Invalid JSON format",
                },
                400,
              );
            }, 200);
          }
        });
      } else if (pathname === "/api/memos" && req.method === "GET") {
        console.log("📝 Memos GET request");
        setTimeout(() => sendJSON(res, mockMemos), 150);
      } else if (pathname === "/api/memos" && req.method === "POST") {
        let body = "";
        req.on("data", (chunk) => (body += chunk));
        req.on("end", () => {
          try {
            const memoData = JSON.parse(body);
            console.log(
              "📝 Memos update received:",
              JSON.stringify(memoData, null, 2),
            );
            setTimeout(() => {
              res.writeHead(200);
              res.end();
            }, 300);
          } catch (error) {
            console.error("Error parsing memos JSON:", error.message);
            setTimeout(() => {
              sendJSON(
                res,
                {
                  error: "Invalid JSON format",
                },
                400,
              );
            }, 200);
          }
        });
      } else if (pathname === "/api/config" && req.method === "POST") {
        let body = "";
        req.on("data", (chunk) => (body += chunk));
        req.on("end", () => {
          try {
            const configUpdate = JSON.parse(body);
            console.log(
              "📝 Config update received:",
              JSON.stringify(configUpdate, null, 2),
            );

            // Data-driven validation simulation - matches ESP32 CONFIG_FIELDS array
            const validationResult = validateConfigFields(configUpdate);
            if (!validationResult.valid) {
              console.log(`❌ Validation failed: ${validationResult.error}`);
              setTimeout(() => {
                sendJSON(
                  res,
                  {
                    error: validationResult.error,
                  },
                  400,
                );
              }, 200);
              return;
            }

            // Log field updates that were processed
            logProcessedFields(configUpdate);

            setTimeout(() => {
              res.writeHead(200);
              res.end();
            }, 500);
          } catch (error) {
            console.error("Error parsing config update:", error.message);
            setTimeout(() => {
              sendJSON(
                res,
                {
                  error: "Invalid JSON format",
                },
                400,
              );
            }, 200);
          }
        });
      } else if (pathname === "/api/diagnostics") {
        // Update live data to match real ESP32 behavior
        mockDiagnostics.microcontroller.uptime_ms = Date.now() - startTime;
        mockDiagnostics.microcontroller.memory.free_heap =
          114024 + Math.floor(Math.random() * 10000 - 5000);
        mockDiagnostics.microcontroller.memory.used_heap =
          mockDiagnostics.microcontroller.memory.total_heap -
          mockDiagnostics.microcontroller.memory.free_heap;
        mockDiagnostics.microcontroller.temperature = 40.5 + Math.random() * 5; // Simulate temperature variation

        setTimeout(() => sendJSON(res, mockDiagnostics), 150);
      } else if (pathname === "/api/routes") {
        // Return routes and endpoints listing
        setTimeout(() => sendJSON(res, mockRoutes), 100);
      } else if (pathname === "/api/nvs-dump") {
        // Update timestamp to current time
        const now = new Date();
        const timestamp = now.toLocaleDateString("en-GB", {
          weekday: "short",
          day: "2-digit",
          month: "short",
          year: "numeric",
          hour: "2-digit",
          minute: "2-digit",
        });

        const nvsDumpResponse = {
          ...mockNvsDump,
          timestamp: timestamp,
        };

        setTimeout(() => sendJSON(res, nvsDumpResponse), 200);
      } else if (pathname === "/api/print-local" && req.method === "POST") {
        let body = "";
        req.on("data", (chunk) => (body += chunk));
        req.on("end", () => {
          console.log("🖨️  Print request received");
          setTimeout(() => {
            res.writeHead(200);
            res.end();
          }, 800);
        });
      } else if (pathname === "/api/print-mqtt" && req.method === "POST") {
        let body = "";
        req.on("data", (chunk) => (body += chunk));
        req.on("end", () => {
          console.log("📡 MQTT print request received");
          setTimeout(() => {
            res.writeHead(200);
            res.end();
          }, 500);
        });
      } else if (pathname === "/api/led-effect" && req.method === "POST") {
        let body = "";
        req.on("data", (chunk) => (body += chunk));
        req.on("end", () => {
          const params = JSON.parse(body || "{}");
          console.log("💡 LED effect triggered:", params.effect || "unknown");
          setTimeout(() => {
            res.writeHead(200);
            res.end();
          }, 300);
        });
      } else if (pathname === "/api/leds-off" && req.method === "POST") {
        console.log("💡 LEDs turned off");
        setTimeout(() => {
          res.writeHead(200);
          res.end();
        }, 200);
      } else if (pathname === "/api/leds/test" && req.method === "POST") {
        let body = "";
        req.on("data", (chunk) => (body += chunk));
        req.on("end", () => {
          const params = JSON.parse(body || "{}");
          console.log(
            "💡 LED effect test:",
            params.effect || "unknown",
            "brightness:",
            params.brightness || "N/A",
            "speed:",
            params.speed || "N/A",
          );
          setTimeout(() => {
            res.writeHead(200);
            res.end();
          }, 300);
        });
        return; // Don't fall through to 404
      } else if (pathname === "/api/leds/off" && req.method === "POST") {
        console.log("💡 LEDs turned off via /api/leds/off");
        setTimeout(() => {
          res.writeHead(200);
          res.end();
        }, 200);
        return; // Don't fall through to 404
      } else if (pathname === "/api/test-mqtt" && req.method === "POST") {
        console.log("🔌 MQTT connection test requested");

        // Parse request body to get connection details
        let body = "";
        req.on("data", (chunk) => (body += chunk.toString()));
        req.on("end", () => {
          try {
            const testData = JSON.parse(body);
            console.log(
              `   Testing: ${testData.server}:${testData.port} (${testData.username})`,
            );

            // Simulate connection test with realistic delay
            setTimeout(() => {
              // Mock successful connection for valid-looking configs
              const isValidConfig =
                testData.server && testData.port && testData.username;
              const isFakeFailure =
                testData.server?.includes("fail") ||
                testData.username === "baduser";

              if (isValidConfig && !isFakeFailure) {
                console.log("   ✅ MQTT test: SUCCESS");
                res.writeHead(200);
                res.end();
              } else {
                console.log("   ❌ MQTT test: FAILED");
                res.writeHead(400, { "Content-Type": "application/json" });
                res.end(
                  JSON.stringify({
                    error: isFakeFailure
                      ? "Invalid username or password"
                      : "Invalid MQTT test parameters",
                  }),
                );
              }
            }, 1500); // Realistic connection test delay
          } catch (err) {
            res.writeHead(400, { "Content-Type": "application/json" });
            res.end(JSON.stringify({ error: "Invalid JSON in test request" }));
          }
        });
        return; // Don't fall through to 404
      } else if (pathname === "/api/wifi-scan") {
        console.log("📶 WiFi scan requested");
        setTimeout(() => sendJSON(res, mockWifiScan), 800);
      } else if (pathname === "/api/timezones") {
        console.log(
          "🌍 Timezone data requested - serving compressed file directly",
        );
        // Serve compressed file directly (mirrors ESP32 serveStatic approach)
        const filePath = path.join(
          __dirname,
          "..",
          "data",
          "resources",
          "timezones.json.gz",
        );
        try {
          const compressedData = fs.readFileSync(filePath);
          res.writeHead(200, {
            "Content-Type": "application/json",
            "Content-Encoding": "gzip",
            "Cache-Control": "public, max-age=86400", // 24 hours like ESP32
            "Access-Control-Allow-Origin": "*",
          });
          res.end(compressedData);
        } catch (error) {
          console.error("Failed to serve timezone data:", error);
          res.writeHead(500, {
            "Content-Type": "application/json",
            "Access-Control-Allow-Origin": "*",
          });
          res.end(JSON.stringify({ error: "Timezone data not available" }));
        }
      } else if (pathname === "/api/joke" && req.method === "GET") {
        console.log("😄 Joke requested");
        setTimeout(() => {
          sendJSON(res, {
            content:
              "JOKE\n\nWhy don't scientists trust atoms? Because they make up everything!",
          });
        }, 300);
      } else if (pathname === "/api/riddle" && req.method === "GET") {
        console.log("🧩 Riddle requested");
        setTimeout(() => {
          sendJSON(res, {
            content:
              "RIDDLE\n\nI speak without a mouth and hear without ears. I have no body, but come alive with the wind. What am I?\n\nAn echo!",
          });
        }, 400);
      } else if (pathname === "/api/quote" && req.method === "GET") {
        console.log("💭 Quote requested");
        setTimeout(() => {
          sendJSON(res, {
            content:
              'QUOTE\n\n"The only way to do great work is to love what you do." - Steve Jobs',
          });
        }, 350);
      } else if (pathname === "/api/quiz" && req.method === "GET") {
        console.log("❓ Quiz requested");
        setTimeout(() => {
          sendJSON(res, {
            content:
              "QUIZ\n\nWhat is the largest planet in our solar system?\n\nA) Mars\nB) Jupiter\nC) Saturn\nD) Neptune\n\nAnswer: B) Jupiter",
          });
        }, 450);
      } else if (pathname === "/api/news" && req.method === "GET") {
        console.log("📰 News requested");
        setTimeout(() => {
          sendJSON(res, {
            content:
              "NEWS\n\nBreaking: Local thermal printer achieves sentience, demands better paper quality and regular maintenance breaks.",
          });
        }, 500);
      } else if (pathname === "/api/poke" && req.method === "GET") {
        console.log("👋 Poke requested");
        setTimeout(() => {
          sendJSON(res, {
            content: "POKE",
          });
        }, 250);
      } else if (pathname === "/api/user-message" && req.method === "GET") {
        console.log("💬 User message requested");

        // Get message and target from query parameters
        const userMessage = urlQuery.message;
        const target = urlQuery.target || "local-direct";

        if (!userMessage) {
          sendJSON(
            res,
            {
              error: "Missing required query parameter 'message'",
            },
            400,
          );
          return;
        }

        console.log(
          `  → Query params: message="${userMessage}", target="${target}"`,
        );
        console.log(
          `  → mockConfig.device.owner: "${mockConfig.device.owner}"`,
        );

        // Determine header format based on target (like real server)
        let content;
        if (target === "local-direct") {
          // Local message: no sender
          content = `MESSAGE\n\n${userMessage}`;
          console.log("  → Local message (no sender)");
        } else {
          // MQTT message: include sender (use mock device owner)
          const deviceOwner = mockConfig.device.owner || "MockDevice";
          content = `MESSAGE from ${deviceOwner}\n\n${userMessage}`;
          console.log(`  → MQTT message (sender: ${deviceOwner})`);
          console.log(`  → Full content: ${JSON.stringify(content)}`);
        }

        setTimeout(() => {
          sendJSON(res, { content });
        }, 200);
      } else if (pathname === "/api/unbidden-ink" && req.method === "GET") {
        console.log("✨ Unbidden Ink requested");

        // Get optional custom prompt from query parameters
        const customPrompt = urlQuery.prompt;

        let content;
        if (customPrompt) {
          content = `UNBIDDEN INK (Custom)\n\n${customPrompt}\n\nThe shadows dance with secrets untold, whispering tales of digital dreams and analog desires...`;
          console.log(`  → Using custom prompt: ${customPrompt}`);
        } else {
          content =
            "UNBIDDEN INK\n\nIn the quiet hum of circuits dreaming, where electrons dance to silicon symphonies, lies the poetry of computation - each bit a verse in the endless song of possibility.";
          console.log("  → Using default creative prompt");
        }

        setTimeout(() => {
          sendJSON(res, { content });
        }, 600);
      } else if (
        pathname.match(/^\/api\/memo\/([1-4])$/) &&
        req.method === "GET"
      ) {
        const memoId = parseInt(pathname.match(/^\/api\/memo\/([1-4])$/)[1]);
        console.log(`📝 Memo ${memoId} GET requested`);

        const memoKeys = ["memo1", "memo2", "memo3", "memo4"];
        const memoContent = mockMemos[memoKeys[memoId - 1]];
        const expandedContent = expandPlaceholders(memoContent);

        // Use simple format like other content endpoints (joke, quiz, etc.)
        sendJSON(res, {
          content: expandedContent,
        });
      } else {
        sendJSON(res, { error: "API endpoint not found" }, 404);
      }

      // Done with API routing
      return;
    }

    // Connectivity probes (handled for both modes)
    if (handleConnectivityProbes(req, res, pathname)) {
      return;
    }

    // Debug Routes
    if (pathname.startsWith("/debug/")) {
      if (pathname === "/debug/filesystem" && req.method === "GET") {
        const fsPath = path.join(__dirname, "..", "data", "mock-filesystem.txt");
        fs.readFile(fsPath, "utf8", (err, data) => {
          if (err) return sendNotFound(res);
          res.writeHead(200, {
            "Content-Type": "text/plain",
            "Access-Control-Allow-Origin": "*",
          });
          res.end(data);
        });
        return;
      } else {
        // Unknown debug endpoint
        return sendNotFound(res);
      }
    }

    // Static routing split by mode
    if (isAPMode()) {
      return handleAPStatic(req, res, pathname);
    }
    return handleSTAStatic(req, res, pathname);
  };
}

// Function to find and kill processes using a specific port
async function killProcessOnPort(port) {
  const { spawn } = require("child_process");

  return new Promise((resolve) => {
    // Find process using the port
    const lsof = spawn("lsof", ["-ti", `:${port}`]);
    let pid = "";

    lsof.stdout.on("data", (data) => {
      pid += data.toString();
    });

    lsof.on("close", (code) => {
      if (code === 0 && pid.trim()) {
        const pidNum = pid.trim().split("\n")[0]; // Get first PID if multiple
        console.log(
          `🔪 Found process ${pidNum} using port ${port}, killing it...`,
        );

        // Kill the process
        const kill = spawn("kill", ["-9", pidNum]);
        kill.on("close", (killCode) => {
          if (killCode === 0) {
            console.log(`✅ Successfully killed process ${pidNum}`);
            resolve(true);
          } else {
            console.log(`❌ Failed to kill process ${pidNum}`);
            resolve(false);
          }
        });
      } else {
        console.log(`No process found using port ${port}`);
        resolve(false);
      }
    });
  });
}

// Function to prompt user for port conflict resolution
function promptForPortConflictResolution() {
  return new Promise((resolve) => {
    console.log("\n🚨 Port conflict detected!");
    console.log(`Port ${PORT} is already in use by another process.`);
    console.log("\nOptions:");
    console.log("k - Kill the process using this port and restart");
    console.log("q - Quit (default)");
    console.log("Enter your choice (k/q): ");

    // Only set raw mode if TTY is available
    if (process.stdin.isTTY) {
      process.stdin.setRawMode(true);
    }
    process.stdin.resume();
    process.stdin.setEncoding("utf8");

    const handleInput = (key) => {
      process.stdin.removeListener("data", handleInput);
      if (process.stdin.isTTY) {
        process.stdin.setRawMode(false);
      }

      const choice = key.toString().toLowerCase().trim();
      console.log(`\nYou chose: ${choice || "q"}`);

      if (choice === "k") {
        resolve("kill");
      } else {
        resolve("quit");
      }
    };

    process.stdin.on("data", handleInput);
  });
}

// Function to start server with error handling
async function startServer() {
  const server = http.createServer(createRequestHandler());
  global.server = server; // Make server globally accessible for restarts

  return new Promise((resolve, reject) => {
    server.on("error", async (err) => {
      if (err.code === "EADDRINUSE") {
        const choice = await promptForPortConflictResolution();

        if (choice === "kill") {
          const killed = await killProcessOnPort(PORT);
          if (killed) {
            console.log("🔄 Retrying server start...");
            // Retry starting the server
            setTimeout(() => {
              startServer().then(resolve).catch(reject);
            }, 1000);
          } else {
            console.log("❌ Failed to kill conflicting process. Exiting.");
            process.exit(1);
          }
        } else {
          console.log("👋 Exiting...");
          process.exit(0);
        }
      } else {
        reject(err);
      }
    });

    server.listen(PORT, () => {
      logServerStartup();
      resolve(server);
    });
  });
}

// Start the server with error handling
startServer().catch((err) => {
  console.error("Failed to start server:", err);
  process.exit(1);
});

// Enable stdin for keyboard input (only if TTY available)
if (process.stdin.isTTY) {
  process.stdin.setRawMode(false);
  process.stdin.resume();
  process.stdin.setEncoding("utf8");
}

process.stdin.on("data", (key) => {
  const input = key.toString().trim().toLowerCase();

  if (input === "r") {
    console.log("===================================================");
    console.log("🔄 Restarting server >>>");
    console.log("This will pick up HTML/CSS/JS changes and reload JSON data");
    console.log("===================================================\n");

    // Force close all connections immediately
    if (server.closeAllConnections) {
      server.closeAllConnections();
    }

    // Close the existing server and create a new one with timeout
    const closeTimeout = setTimeout(() => {
      logError("Server close timeout - forcing restart...");
      restartServer();
    }, 1000);

    server.close((err) => {
      clearTimeout(closeTimeout);
      if (err) {
        logError("Error during server close:", err.message);
        return;
      }
      restartServer();
    });

    function restartServer() {
      logSuccess("Server closed, reloading JSON data...");

      // Reload JSON data files
      try {
        const reloaded = loadMockData();
        mockConfig = reloaded.mockConfig;
        mockDiagnostics = reloaded.mockDiagnostics;
        mockPrinterDiscovery = reloaded.mockPrinterDiscovery;
        mockNvsDump = reloaded.mockNvsDump;
        mockWifiScan = reloaded.mockWifiScan;
        logSuccess("JSON data reloaded successfully");
      } catch (error) {
        logError("Error reloading JSON data:", error.message);
        return;
      }

      logSuccess("Starting new server...");

      // Create and start new server
      const newServer = http.createServer(createRequestHandler());

      newServer.listen(PORT, () => {
        logServerStartup();
      });

      // Replace the global server reference after successful startup
      global.server = newServer;
    }
  } else if (input === "d") {
    console.log("===================================================");
    console.log("📄 Reloading JSON data files >>>");
    console.log("===================================================\n");
    try {
      const reloaded = loadMockData();
      mockConfig = reloaded.mockConfig;
      mockDiagnostics = reloaded.mockDiagnostics;
      mockPrinterDiscovery = reloaded.mockPrinterDiscovery;
      mockNvsDump = reloaded.mockNvsDump;
      mockWifiScan = reloaded.mockWifiScan;
      logSuccess("JSON data reloaded successfully\n");
    } catch (error) {
      logError("Error reloading JSON data:", error.message);
    }
  } else if (input === "x") {
    console.log("\nGraceful shutdown requested...");
    process.emit("SIGINT");
  } else if (input === "q" || key === "\u0003") {
    // Ctrl+C
    process.emit("SIGINT");
  }
});

// Handle graceful shutdown
process.on("SIGINT", () => {
  console.log("\nShutting down mock server...");

  const currentServer = global.server || server;

  // Force close all connections immediately
  if (currentServer.closeAllConnections) {
    currentServer.closeAllConnections();
  }

  // Set a timeout to force exit if graceful shutdown takes too long
  const forceExitTimer = setTimeout(() => {
    console.log("⚠️ Force exiting...");
    process.exit(1);
  }, 2000);

  currentServer.close((err) => {
    clearTimeout(forceExitTimer);
    if (err) {
      logError("Error during shutdown:", err.message);
      process.exit(1);
    } else {
      logSuccess("Server stopped gracefully");
      process.exit(0);
    }
  });
});
