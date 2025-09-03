const fs = require("fs");
const path = require("path");
const { sendNotFound } = require("../utils/respond");

function handleDebug(pathname, res) {
  if (pathname !== "/debug/filesystem") return false;
  const fsPath = path.join(__dirname, "..", "data", "mock-filesystem.txt");
  fs.readFile(fsPath, "utf8", (err, data) => {
    if (err) return sendNotFound(res);
    res.writeHead(200, {
      "Content-Type": "text/plain",
      "Access-Control-Allow-Origin": "*",
    });
    res.end(data);
  });
  return true;
}

module.exports = { handleDebug };
