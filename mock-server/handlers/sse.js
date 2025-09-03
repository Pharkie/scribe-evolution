function handleSSE(req, res, mockPrinterDiscovery) {
  res.writeHead(200, {
    "Content-Type": "text/event-stream",
    "Cache-Control": "no-cache",
    Connection: "keep-alive",
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Headers": "Cache-Control",
  });

  const data = { ...mockPrinterDiscovery };
  if (!Array.isArray(data.discovered_printers)) {
    data.discovered_printers = [];
  }
  res.write(`event: printer-update\ndata: ${JSON.stringify(data)}\n\n`);
  const interval = setInterval(() => {
    if (res.destroyed) return clearInterval(interval);
    if (Array.isArray(data.discovered_printers) && data.discovered_printers.length > 0) {
      data.discovered_printers[0].last_power_on = new Date().toISOString();
    }
    res.write(`event: printer-update\ndata: ${JSON.stringify(data)}\n\n`);
  }, 30000);
  req.on("close", () => clearInterval(interval));
}

module.exports = { handleSSE };
