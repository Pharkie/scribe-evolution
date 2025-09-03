#!/usr/bin/env node

/**
 * Mock API Server for Scribe Evolution Local Development
 * Minimal bootstrap that delegates to handlers and utils.
 * Usage: node mock-api.js [--ap-mode|--no-leds|--disable-mqtt]
 */

const http = require("http");
const readline = require("readline");
const { execSync } = require("child_process");
const { createRequestHandler } = require("./router");
const { loadMockData } = require("./utils/data");
const {
  validateConfigFields,
  logProcessedFields,
} = require("./utils/validation");
const { sendNotFound } = require("./utils/respond");

const startTime = Date.now();
let server = null;

// Determine mode from CLI args
const args = process.argv.slice(2);
// Support --port <n> or --port=<n> or env PORT
function parsePort(argv) {
  let val = null;
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === "--port" || a === "-p") {
      const n = Number(argv[i + 1]);
      if (!Number.isNaN(n)) val = n;
    } else if (a.startsWith("--port=")) {
      const n = Number(a.split("=", 2)[1]);
      if (!Number.isNaN(n)) val = n;
    }
  }
  return val || Number(process.env.PORT) || 3001;
}
const port = parsePort(args);
const quiet = args.includes("--quiet") || process.env.QUIET === "1";

const currentMode = args.includes("--ap-mode")
  ? "ap-mode"
  : args.includes("--no-leds")
    ? "no-leds"
    : args.includes("--disable-mqtt")
      ? "disable-mqtt"
      : "normal";

// Load data
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

function startServer() {
  const ctx = {
    // data
    mockConfig,
    mockConfigAPMode,
    mockConfigNoLEDs,
    mockDiagnostics,
    mockPrinterDiscovery,
    mockNvsDump,
    mockWifiScan,
    mockMemos,
    mockRoutes,
    // helpers
    validateConfigFields,
    logProcessedFields,
    startTime,
    currentMode,
    isAPMode: () => currentMode === "ap-mode",
    sendNotFound,
  };

  server = http.createServer(createRequestHandler(ctx));
  server.listen(port, () => {
    printStartupHelp();
    setupKeyboardShortcuts();
  });
}

function restartServer() {
  if (!server) return startServer();
  console.log("\n🔄 Restarting mock server...");
  const t = setTimeout(() => startServer(), 1000);
  server.close(() => {
    clearTimeout(t);
    startServer();
  });
}

function gracefulShutdown() {
  console.log("\nShutting down mock server...");
  const t = setTimeout(() => process.exit(0), 1000);
  if (!server) return process.exit(0);
  server.close(() => {
    clearTimeout(t);
    console.log("✅ Server stopped. Bye!");
    process.exit(0);
  });
}

function printStartupHelp() {
  if (quiet) return;
  console.log("===================================================");
  console.log(`🚀 Mock server started at: http://localhost:${port}`);
  console.log(`📦 Mode: ${currentMode}`);
  console.log("");
  console.log("Usage:");
  console.log("  node mock-api.js                      # STA-like mode");
  console.log(
    "  node mock-api.js --ap-mode            # AP captive-portal mode",
  );
  console.log(
    "  node mock-api.js --no-leds            # STA mode with LEDs disabled",
  );
  console.log(
    "  node mock-api.js --disable-mqtt       # STA mode with MQTT disabled",
  );
  console.log("  node mock-api.js --port 3010          # Custom port");
  console.log("  node mock-api.js --quiet              # Suppress banner/logs");
  console.log("");
  console.log("Keyboard shortcuts:");
  console.log(
    "  r  Reload JSON data files (config, diagnostics, routes, etc.)",
  );
  console.log("  s  Restart server (close + rebind listener)");
  console.log("  x  Quit (graceful)");
  console.log("  Ctrl+C  Quit (graceful)");
  console.log("===================================================");
}

function setupKeyboardShortcuts() {
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
    if (input === "s") {
      restartServer();
      return;
    }
    if (input === "x") {
      gracefulShutdown();
      return;
    }
  });
  process.on("SIGINT", () => gracefulShutdown());
}

// Boot
init();

async function init() {
  try {
    await checkAndResolvePortInUse();
  } catch (e) {
    console.error(e?.message || e);
    process.exit(1);
  }
  startServer();
}

function getPidsOnPort(p) {
  try {
    const out = execSync(`lsof -ti :${p}`, {
      stdio: ["ignore", "pipe", "ignore"],
    })
      .toString()
      .trim();
    if (!out) return [];
    const ids = Array.from(new Set(out.split(/\s+/).filter(Boolean)))
      .map((s) => parseInt(s, 10))
      .filter((n) => !Number.isNaN(n));
    return ids;
  } catch {
    return [];
  }
}

function askYesNo(question, defaultNo = true) {
  return new Promise((resolve) => {
    if (!process.stdin.isTTY || quiet) return resolve(false);
    const rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout,
    });
    const suffix = defaultNo ? " [y/N]: " : " [Y/n]: ";
    rl.question(question + suffix, (ans) => {
      rl.close();
      const a = (ans || "").trim().toLowerCase();
      if (!a) return resolve(!defaultNo);
      resolve(a === "y" || a === "yes");
    });
  });
}

async function checkAndResolvePortInUse() {
  const pids = getPidsOnPort(port);
  if (pids.length === 0) return;
  if (quiet || !process.stdin.isTTY) {
    throw new Error(
      `Port ${port} is in use by PID(s) ${pids.join(", ")}. Re-run with --port <n> or free the port.`,
    );
  }
  console.log(`⚠️  Port ${port} is in use by PID(s): ${pids.join(", ")}`);
  const kill = await askYesNo(
    "Do you want me to kill these processes now?",
    true,
  );
  if (!kill)
    throw new Error(
      "Aborting due to port conflict. Use --port to choose a different port.",
    );
  let killed = 0;
  for (const pid of pids) {
    try {
      process.kill(pid, "SIGKILL");
      killed++;
    } catch {}
  }
  if (killed === 0) throw new Error("Failed to kill processes using the port.");
  // Give OS a moment to release the socket
  await new Promise((r) => setTimeout(r, 300));
}
