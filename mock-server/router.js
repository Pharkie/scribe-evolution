const path = require("path");
const fs = require("fs");
const { handleAPI } = require("./handlers/api");
const { handleSSE } = require("./handlers/sse");
const { handleAPStatic, handleSTAStatic } = require("./handlers/static");
const { handleConnectivity, isProbe } = require("./handlers/connectivity");
const { handleDebug } = require("./handlers/debug");

function createRequestHandler(ctx) {
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
      return handleSSE(req, res, ctx.mockPrinterDiscovery);
    }

    // Connectivity checks
    if (handleConnectivity(pathname, req, res, ctx.isAPMode())) return;

    // Favicon assets
    if (
      pathname === "/favicon.ico" ||
      pathname === "/favicon-96x96.png" ||
      pathname === "/apple-touch-icon.png"
    ) {
      const filePath = path.join(__dirname, "..", "data", pathname);
      fs.readFile(filePath, (err, data) => {
        if (err) return ctx.sendNotFound(res);
        const contentType = pathname.endsWith(".ico")
          ? "image/x-icon"
          : "image/png";
        res.writeHead(200, {
          "Content-Type": contentType,
          "Cache-Control": "public, max-age=604800",
          "Access-Control-Allow-Origin": "*",
        });
        res.end(data);
      });
      return;
    }

    // API
    if (pathname.startsWith("/api/")) {
      if (handleAPI(req, res, pathname, ctx)) return;
      return;
    }

    // Debug
    if (handleDebug(pathname, res)) return;

    // Static by mode
    if (ctx.isAPMode()) return handleAPStatic(pathname, res);
    return handleSTAStatic(pathname, res);
  };
}

module.exports = { createRequestHandler };
