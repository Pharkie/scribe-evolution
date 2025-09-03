const { serveText } = require("../utils/respond");

function isProbe(pathname) {
  return (
    pathname === "/hotspot-detect.html" ||
    pathname === "/generate_204" ||
    pathname === "/connectivity-check.html" ||
    pathname === "/ncsi.txt"
  );
}

function handleConnectivity(pathname, req, res, apMode) {
  if (!isProbe(pathname)) return false;
  if (apMode) {
    res.writeHead(302, {
      Location: "/setup.html",
      "Access-Control-Allow-Origin": "*",
    });
    res.end("Redirecting to setup page...");
    return true;
  }
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

module.exports = { isProbe, handleConnectivity };
