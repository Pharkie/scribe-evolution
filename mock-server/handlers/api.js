const fs = require("fs");
const path = require("path");
const { sendJSON } = require("../utils/respond");

function handleAPI(req, res, pathname, ctx) {
  if (!pathname.startsWith("/api/")) return false;

  const {
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
  } = ctx;

  // /api/test-wifi (POST)
  if (pathname === "/api/test-wifi" && req.method === "POST") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        const { ssid, password } = JSON.parse(body || "{}");
        if (!ssid || typeof ssid !== "string" || ssid.trim() === "") {
          return sendJSON(
            res,
            { success: false, message: "Invalid payload" },
            422,
          );
        }

        if (global.__wifiTestBusy) {
          return sendJSON(
            res,
            { success: false, message: "Test already running" },
            409,
          );
        }
        global.__wifiTestBusy = true;

        const simulateLong =
          new URL(req.url, `http://${req.headers.host}`).searchParams.get(
            "long",
          ) === "1";
        const delayMs = simulateLong ? 6500 : 800;

        setTimeout(() => {
          global.__wifiTestBusy = false;
          const pwd = password || "";
          if (pwd.includes("timeout"))
            return sendJSON(
              res,
              { success: false, message: "Association timeout" },
              408,
            );
          if (pwd.includes("noap"))
            return sendJSON(
              res,
              { success: false, message: "No AP found" },
              400,
            );
          if (pwd.includes("auth"))
            return sendJSON(
              res,
              { success: false, message: "Authentication failed" },
              400,
            );
          return sendJSON(res, { success: true, rssi: -52 }, 200);
        }, delayMs);
      } catch (e) {
        return sendJSON(
          res,
          { success: false, message: "Invalid payload" },
          422,
        );
      }
    });
    return true;
  }

  // /api/test-chatgpt (POST)
  if (pathname === "/api/test-chatgpt" && req.method === "POST") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        const { token } = JSON.parse(body || "{}");
        if (!token || typeof token !== "string") {
          return sendJSON(
            res,
            { success: false, error: "Missing 'token'" },
            400,
          );
        }
        // Simulate success for any token except 'badkey'
        if (token === "badkey") {
          return sendJSON(
            res,
            { success: false, error: "Invalid API key" },
            401,
          );
        }
        return sendJSON(res, { success: true }, 200);
      } catch (e) {
        return sendJSON(
          res,
          { success: false, error: "Invalid JSON format" },
          400,
        );
      }
    });
    return true;
  }

  // /api/config (GET)
  if (pathname === "/api/config" && req.method === "GET") {
    const mode =
      new URL(req.url, `http://${req.headers.host}`).searchParams.get("mode") ||
      ctx.currentMode;
    let configToSend = mockConfig;
    if (mode === "ap-mode") configToSend = mockConfigAPMode;
    else if (mode === "no-leds") configToSend = mockConfigNoLEDs;
    else if (mode === "disable-mqtt")
      configToSend = {
        ...mockConfig,
        mqtt: { ...mockConfig.mqtt, enabled: false },
      };
    return (sendJSON(res, configToSend, 200), true);
  }

  // /api/config (POST)
  if (pathname === "/api/config" && req.method === "POST") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        const configUpdate = JSON.parse(body);
        const validationResult = validateConfigFields(configUpdate);
        if (!validationResult.valid) {
          return sendJSON(res, { error: validationResult.error }, 400);
        }
        logProcessedFields(configUpdate);

        // Simulate WiFi credential change detection (in STA mode only)
        if (!ctx.isAPMode() && configUpdate.wifi) {
          const currentConfig = mockConfig; // Current mock config
          let wifiCredentialsChanged = false;

          // Check if SSID changed
          if (
            configUpdate.wifi.ssid &&
            configUpdate.wifi.ssid !== currentConfig.device.wifi.ssid
          ) {
            wifiCredentialsChanged = true;
            console.log(
              `📡 Mock: WiFi SSID changed from '${currentConfig.device.wifi.ssid}' to '${configUpdate.wifi.ssid}'`,
            );
          }

          // Check if password changed (if provided)
          if (configUpdate.wifi.password) {
            wifiCredentialsChanged = true;
            console.log(`📡 Mock: WiFi password changed`);
          }

          // If WiFi credentials changed, return restart signal
          if (wifiCredentialsChanged) {
            console.log(`📡 Mock: Simulating device restart for WiFi changes`);
            return sendJSON(res, { restart: true, reason: "wifi_change" }, 200);
          }
        }

        // Normal config save (no restart)
        res.writeHead(200);
        res.end();
      } catch (error) {
        return sendJSON(res, { error: "Invalid JSON format" }, 400);
      }
    });
    return true;
  }

  // /api/leds/test (POST)
  if (pathname === "/api/leds/test" && req.method === "POST") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        JSON.parse(body || "{}");
      } catch (e) {
        return sendJSON(res, { error: "Invalid JSON format" }, 400);
      }
      // Simulate short processing time
      setTimeout(() => {
        res.writeHead(200);
        res.end();
      }, 150);
    });
    return true;
  }

  // /api/leds/off (POST)
  if (pathname === "/api/leds/off" && req.method === "POST") {
    setTimeout(() => {
      res.writeHead(200);
      res.end();
    }, 100);
    return true;
  }

  // /api/wifi-scan (GET)
  if (pathname === "/api/wifi-scan") {
    setTimeout(() => sendJSON(res, mockWifiScan), 800);
    return true;
  }

  // /api/setup (AP mode provisioning) - ONLY in AP mode
  if (pathname === "/api/setup" && req.method === "GET") {
    if (!ctx.isAPMode())
      return (sendJSON(res, { error: "Not available in STA mode" }, 404), true);
    const setupConfig = {
      device: { owner: "", timezone: "" },
      wifi: { ssid: "", password: "" },
    };
    return (sendJSON(res, setupConfig, 200), true);
  }

  if (pathname === "/api/setup" && req.method === "POST") {
    if (!ctx.isAPMode())
      return (sendJSON(res, { error: "Not available in STA mode" }, 404), true);
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        JSON.parse(body || "{}");
        res.writeHead(200);
        res.end();
      } catch (e) {
        return sendJSON(res, { error: "Invalid JSON format" }, 400);
      }
    });
    return true;
  }

  // Content endpoints - structured data format (header + body)
  const contentEndpoints = {
    "/api/joke": {
      delay: 300,
      body: {
        header: "JOKE",
        body: "Why don't scientists trust atoms? Because they make up everything!",
      },
    },
    "/api/riddle": {
      delay: 400,
      body: {
        header: "RIDDLE",
        body: "I speak without a mouth and hear without ears. I have no body, but come alive with the wind. What am I?\n\nAn echo!",
      },
    },
    "/api/quote": {
      delay: 350,
      body: {
        header: "QUOTE",
        body: '"The only way to do great work is to love what you do." - Steve Jobs',
      },
    },
    "/api/quiz": {
      delay: 450,
      body: {
        header: "QUIZ",
        body: "What is the largest planet in our solar system?\n\nA) Mars\nB) Jupiter\nC) Saturn\nD) Neptune\n\nAnswer: B) Jupiter",
      },
    },
    "/api/news": {
      delay: 500,
      body: {
        header: "NEWS",
        body: "Breaking: Local thermal printer achieves sentience, demands better paper quality and regular maintenance breaks.",
      },
    },
    "/api/poke": {
      delay: 250,
      body: {
        header: "POKE",
        body: "",
      },
    },
  };

  if (contentEndpoints[pathname] && req.method === "GET") {
    const { delay, body } = contentEndpoints[pathname];
    setTimeout(() => sendJSON(res, body), delay);
    return true;
  }

  if (pathname === "/api/user-message" && req.method === "GET") {
    const url = new URL(req.url, `http://${req.headers.host}`);
    const userMessage = url.searchParams.get("message");
    const target = url.searchParams.get("target") || "local-direct";
    if (!userMessage)
      return (
        sendJSON(
          res,
          { error: "Missing required query parameter 'message'" },
          400,
        ),
        true
      );
    let header, body;
    header = "MESSAGE";
    body = userMessage;
    setTimeout(() => sendJSON(res, { header, body }), 200);
    return true;
  }

  if (pathname === "/api/unbidden-ink" && req.method === "GET") {
    const url = new URL(req.url, `http://${req.headers.host}`);
    const customPrompt = url.searchParams.get("prompt");
    let header, body;
    if (customPrompt) {
      header = "UNBIDDEN INK (Custom)";
      body = `${customPrompt}\n\nThe shadows dance with secrets untold, whispering tales of digital dreams and analog desires...`;
    } else {
      header = "UNBIDDEN INK";
      body =
        "In the quiet hum of circuits dreaming, where electrons dance to silicon symphonies, lies the poetry of computation - each bit a verse in the endless song of possibility.";
    }
    setTimeout(() => sendJSON(res, { header, body }), 600);
    return true;
  }

  // Placeholder expansion function to mimic ESP32 behavior
  function expandPlaceholders(content) {
    return content.replace(/\[([^\]]+)\]/g, (match, placeholder) => {
      const lowerPlaceholder = placeholder.toLowerCase();

      // Simple placeholders
      if (lowerPlaceholder === "date") {
        return new Date().toLocaleDateString("en-GB", {
          weekday: "short",
          day: "2-digit",
          month: "short",
          year: "numeric",
        });
      } else if (lowerPlaceholder === "time") {
        return new Date().toLocaleTimeString("en-GB", {
          hour: "2-digit",
          minute: "2-digit",
        });
      } else if (lowerPlaceholder === "weekday") {
        return new Date().toLocaleDateString("en-GB", { weekday: "long" });
      } else if (lowerPlaceholder === "coin") {
        return Math.random() < 0.5 ? "heads" : "tails";
      } else if (lowerPlaceholder === "uptime") {
        const uptimeMs = Date.now() - startTime;
        const hours = Math.floor(uptimeMs / (1000 * 60 * 60));
        const mins = Math.floor((uptimeMs % (1000 * 60 * 60)) / (1000 * 60));
        return `${hours}h ${mins}m`;
      } else if (lowerPlaceholder === "ip") {
        return "192.168.1.123"; // Mock IP
      } else if (lowerPlaceholder === "mdns") {
        return "scribe-printer.local"; // Mock mDNS
      } else if (lowerPlaceholder === "dice") {
        return Math.floor(Math.random() * 6) + 1;
      }

      // Complex placeholders
      else if (lowerPlaceholder.startsWith("dice:")) {
        const max = parseInt(lowerPlaceholder.split(":")[1]);
        if (max && max > 0) {
          return Math.floor(Math.random() * max) + 1;
        }
      } else if (lowerPlaceholder.startsWith("pick:")) {
        const options = lowerPlaceholder.substring(5).split("|");
        if (options.length > 0) {
          return options[Math.floor(Math.random() * options.length)];
        }
      }

      // Return original placeholder if not recognized
      return match;
    });
  }

  if (pathname.match(/^\/api\/memo\/([1-4])$/) && req.method === "GET") {
    const memoId = parseInt(pathname.match(/^\/api\/memo\/([1-4])$/)[1]);
    const memoKeys = ["memo1", "memo2", "memo3", "memo4"];
    const memoContent = mockMemos[memoKeys[memoId - 1]];
    const expandedContent = expandPlaceholders(memoContent);
    return (
      sendJSON(res, { header: `MEMO ${memoId}`, body: expandedContent }),
      true
    );
  }

  if (pathname === "/api/memos" && req.method === "GET") {
    setTimeout(() => sendJSON(res, mockMemos), 150);
    return true;
  }

  if (pathname === "/api/memos" && req.method === "POST") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        JSON.parse(body);
        setTimeout(() => {
          res.writeHead(200);
          res.end();
        }, 300);
      } catch (error) {
        setTimeout(
          () => sendJSON(res, { error: "Invalid JSON format" }, 400),
          200,
        );
      }
    });
    return true;
  }

  if (pathname === "/api/diagnostics") {
    mockDiagnostics.microcontroller.uptime_ms = Date.now() - startTime;
    mockDiagnostics.microcontroller.memory.free_heap =
      114024 + Math.floor(Math.random() * 10000 - 5000);
    mockDiagnostics.microcontroller.memory.used_heap =
      mockDiagnostics.microcontroller.memory.total_heap -
      mockDiagnostics.microcontroller.memory.free_heap;
    mockDiagnostics.microcontroller.temperature = 40.5 + Math.random() * 5;
    setTimeout(() => sendJSON(res, mockDiagnostics), 150);
    return true;
  }

  if (pathname === "/api/routes") {
    // Load mock routes from mock-server/data (not firmware data/)
    const routesPath = path.join(__dirname, "..", "data", "mock-routes.json");
    try {
      const text = fs.readFileSync(routesPath, "utf8");
      const data = JSON.parse(text);
      setTimeout(() => sendJSON(res, data), 100);
    } catch (e) {
      // Fail fast: surface explicit error, do not fallback
      return sendJSON(res, { error: "mock-routes.json not available" }, 500);
    }
    return true;
  }

  if (pathname === "/api/nvs-dump") {
    const nvsData = {
      ...mockNvsDump,
      timestamp: new Date().toLocaleString("en-GB", {
        weekday: "short",
        day: "2-digit",
        month: "short",
        year: "numeric",
        hour: "2-digit",
        minute: "2-digit",
      }),
    };
    setTimeout(() => sendJSON(res, nvsData), 200);
    return true;
  }

  if (pathname === "/api/print-local" && req.method === "POST") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      setTimeout(() => {
        res.writeHead(200);
        res.end();
      }, 800);
    });
    return true;
  }

  if (pathname === "/api/print-mqtt" && req.method === "POST") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      setTimeout(() => {
        res.writeHead(200);
        res.end();
      }, 800);
    });
    return true;
  }

  // /api/test-mqtt (POST) — validate payload and simulate success/busy/long
  if (pathname === "/api/test-mqtt" && req.method === "POST") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      let payload;
      try {
        payload = JSON.parse(body || "{}");
      } catch (e) {
        return sendJSON(res, { error: "Invalid JSON format" }, 400);
      }

      const url = new URL(req.url, `http://${req.headers.host}`);
      const simulateLong = url.searchParams.get("long") === "1";
      const forceBusy = url.searchParams.get("busy") === "1";

      const server = payload.server;
      const port = Number(payload.port);
      const username = payload.username;

      if (!server || typeof server !== "string" || server.trim() === "") {
        return sendJSON(res, { error: "MQTT server cannot be blank" }, 400);
      }
      if (!Number.isInteger(port) || port < 1 || port > 65535) {
        return sendJSON(res, { error: "Port must be between 1-65535" }, 400);
      }
      if (!username || typeof username !== "string" || username.trim() === "") {
        return sendJSON(res, { error: "Username cannot be blank" }, 400);
      }

      if (forceBusy || global.__mqttTestBusy) {
        return sendJSON(res, { error: "Test already running" }, 409);
      }
      global.__mqttTestBusy = true;

      const delayMs = simulateLong ? 2000 : 300;
      setTimeout(() => {
        global.__mqttTestBusy = false;
        // Failure triggers: server contains 'fail' OR username is 'baduser'
        if (String(server).includes("fail")) {
          return sendJSON(res, { error: "Connection test failed" }, 400);
        }
        if (String(username).toLowerCase() === "baduser") {
          return sendJSON(res, { error: "Unauthorized" }, 401);
        }
        return sendJSON(
          res,
          { success: true, message: "Successfully connected to MQTT broker" },
          200,
        );
      }, delayMs);
    });
    return true;
  }

  // Unknown API endpoint
  sendJSON(res, { error: "API endpoint not found" }, 404);
  return true;
}

module.exports = { handleAPI };
