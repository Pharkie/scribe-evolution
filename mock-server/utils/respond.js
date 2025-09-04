const fs = require("fs");
const path = require("path");

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
  // Candidate locations for 404 page (priority order):
  // 1) mock-server/data/404.html(.gz)
  // 2) repo data/404.html(.gz) from build output
  // 3) source template src/web-static/404.html
  const candidates = [];
  const mockData = path.join(__dirname, "..", "data");
  const repoData = path.join(__dirname, "..", "..", "data");
  const src404 = path.join(
    __dirname,
    "..",
    "..",
    "src",
    "web-static",
    "404.html",
  );

  candidates.push({ file: path.join(mockData, "404.html.gz"), gzip: true });
  candidates.push({ file: path.join(mockData, "404.html"), gzip: false });
  candidates.push({ file: path.join(repoData, "404.html.gz"), gzip: true });
  candidates.push({ file: path.join(repoData, "404.html"), gzip: false });
  candidates.push({ file: src404, gzip: false });

  for (const c of candidates) {
    if (fs.existsSync(c.file)) {
      return fs.readFile(c.file, (err, data) => {
        if (err) breakFallback();
        const headers = {
          "Content-Type": "text/html",
          "Access-Control-Allow-Origin": "*",
          "Cache-Control": "no-cache",
        };
        if (c.gzip) headers["Content-Encoding"] = "gzip";
        res.writeHead(404, headers);
        return res.end(data);
      });
    }
  }

  function breakFallback() {
    res.writeHead(404, {
      "Content-Type": "text/html",
      "Access-Control-Allow-Origin": "*",
    });
    res.end("<h1>404 Not Found</h1>");
  }

  return breakFallback();
}

function serveFile(res, filePath, statusCode = 200, opts = {}) {
  const fsPromises = fs.promises;
  const ext = path.extname(filePath).toLowerCase();
  const mimeTypes = {
    ".html": "text/html",
    ".css": "text/css",
    ".js": "application/javascript",
    ".json": "application/json",
    ".svg": "image/svg+xml",
    ".txt": "text/plain",
    ".md": "text/markdown",
    ".ico": "image/x-icon",
    ".png": "image/png",
    ".webmanifest": "application/manifest+json",
    ".woff2": "font/woff2",
  };
  const contentType = mimeTypes[ext] || "application/octet-stream";

  const compressibleTypes = [
    ".html",
    ".css",
    ".js",
    ".json",
    ".svg",
    ".txt",
    ".md",
  ];
  const isTextType = compressibleTypes.includes(ext);
  const isCoreWebAsset = ext === ".html" || ext === ".css" || ext === ".js";
  const apMode = !!opts.apMode;
  // Never attempt gzip resolution for font files (avoid .woff2.gz lookup or creation attempts)
  if (ext === ".woff2") {
    return fs.readFile(filePath, (err, data) => {
      if (err) return sendNotFound(res);
      res.writeHead(statusCode, {
        "Content-Type": contentType,
        "Access-Control-Allow-Origin": "*",
        "Cache-Control": apMode ? "no-cache" : "public, max-age=31536000",
      });
      res.end(data);
    });
  }
  const shouldCompress = isTextType && !(apMode && isCoreWebAsset);

  if (shouldCompress) {
    const gzipPath = filePath + ".gz";
    fs.readFile(gzipPath, (err, data) => {
      if (err) return sendNotFound(res);
      res.writeHead(statusCode, {
        "Content-Type": contentType,
        "Content-Encoding": "gzip",
        "Access-Control-Allow-Origin": "*",
        "Cache-Control": "public, max-age=31536000",
      });
      res.end(data);
    });
    return;
  }

  fs.readFile(filePath, (err, data) => {
    if (err) return sendNotFound(res);
    res.writeHead(statusCode, {
      "Content-Type": contentType,
      "Access-Control-Allow-Origin": "*",
      "Cache-Control": apMode ? "no-cache" : "public, max-age=31536000",
    });
    res.end(data);
  });
}

function serveFileOr404(res, filePath, opts = {}) {
  // Debug check path existence
  // console.log('[MOCK] static check', filePath, fs.existsSync(filePath), fs.existsSync(filePath + '.gz'));
  if (fs.existsSync(filePath) || fs.existsSync(filePath + ".gz")) {
    return serveFile(res, filePath, 200, opts);
  }
  return sendNotFound(res);
}

function serveText(res, status, contentType, body) {
  res.writeHead(status, {
    "Content-Type": contentType,
    "Access-Control-Allow-Origin": "*",
  });
  res.end(body);
}

module.exports = {
  sendJSON,
  sendNotFound,
  serveFile,
  serveFileOr404,
  serveText,
};
