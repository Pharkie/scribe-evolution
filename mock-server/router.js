const path = require("path");
const fs = require("fs");
const { handleAPI } = require("./handlers/api");
const { handleSSE } = require("./handlers/sse");
const { handleAPStatic, handleSTAStatic } = require("./handlers/static");
const { handleConnectivity, isProbe } = require("./handlers/connectivity");
const { handleDebug } = require("./handlers/debug");
const { sendJSON } = require("./utils/respond");
const { authenticatedHandler, handleSessionCreation } = require("./utils/auth");

function createRequestHandler(ctx) {
  return (req, res) => {
    let pathname = "/";
    try {
      pathname = new URL(req.url, `http://${req.headers.host}`).pathname;
    } catch {}
    try {
      // Handle session creation for index page access
      handleSessionCreation(req, res, () => {});

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
        return authenticatedHandler(
          req,
          res,
          (req, res) => {
            handleAPI(req, res, pathname, ctx);
          },
          ctx,
        );
      }

      // Debug
      if (handleDebug(pathname, res)) return;

      // Static by mode
      if (ctx.isAPMode()) return handleAPStatic(pathname, res);
      return handleSTAStatic(pathname, res);
    } catch (e) {
      console.error("[mock] Unhandled error:", e?.message || e);
      if ((req.url || "").startsWith("/api/")) {
        return sendJSON(
          res,
          { error: "Internal mock error", details: String(e?.message || e) },
          500,
        );
      }
      return ctx.sendNotFound(res);
    }
  };
}

module.exports = { createRequestHandler };
