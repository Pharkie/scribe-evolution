function handleSSE(req, res, mockPrinterDiscovery) {
  console.log("📡 Mock: SSE /mqtt-printers connection established");

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

  // Send initial data immediately
  const initialPayload = `event: printer-update\ndata: ${JSON.stringify(data)}\n\n`;
  console.log("📡 Mock: Sending initial SSE data:", data);
  res.write(initialPayload);

  // Set up periodic updates
  const interval = setInterval(() => {
    if (res.destroyed) {
      console.log("📡 Mock: SSE connection destroyed, clearing interval");
      return clearInterval(interval);
    }
    if (
      Array.isArray(data.discovered_printers) &&
      data.discovered_printers.length > 0
    ) {
      data.discovered_printers[0].last_power_on = new Date().toISOString();
    }
    const updatePayload = `event: printer-update\ndata: ${JSON.stringify(data)}\n\n`;
    console.log("📡 Mock: Sending SSE update");
    res.write(updatePayload);
  }, 30000);

  req.on("close", () => {
    console.log("📡 Mock: SSE client disconnected");
    clearInterval(interval);
  });

  res.on("error", (err) => {
    console.error("📡 Mock: SSE response error:", err);
    clearInterval(interval);
  });
}

module.exports = { handleSSE };
