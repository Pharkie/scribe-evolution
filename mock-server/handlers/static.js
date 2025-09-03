const path = require("path");
const { serveFileOr404, sendNotFound } = require("../utils/respond");

function handleAPStatic(pathname, res) {
  if (pathname === "/") {
    const p = path.join(__dirname, "..", "..", "data", "setup.html");
    return serveFileOr404(res, p, { apMode: true });
  }
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
  const ok = allowed.some(
    (pfx) => pathname === pfx || pathname.startsWith(pfx),
  );
  if (!ok) {
    res.writeHead(302, { Location: "/setup.html" });
    res.end();
    return;
  }
  const reqPath = pathname.substring(1);
  const filePath = path.join(__dirname, "..", "..", "data", reqPath);
  serveFileOr404(res, filePath, { apMode: true });
}

function handleSTAStatic(pathname, res) {
  if (pathname === "/") {
    const p = path.join(__dirname, "..", "..", "data", "index.html");
    return serveFileOr404(res, p);
  }
  if (pathname === "/setup.html") return sendNotFound(res);
  if (pathname.endsWith("/")) {
    const dir = pathname.substring(1, pathname.length - 1);
    const indexPath = path.join(
      __dirname,
      "..",
      "..",
      "data",
      dir,
      "index.html",
    );
    return serveFileOr404(res, indexPath);
  }
  const reqPath = pathname.substring(1) || "index.html";
  const filePath = path.join(__dirname, "..", "..", "data", reqPath);
  serveFileOr404(res, filePath);
}

module.exports = { handleAPStatic, handleSTAStatic };
