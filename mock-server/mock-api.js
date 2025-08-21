#!/usr/bin/env node

/**
 * Mock API Server for Scribe Evolution Local Development
 * Uses only Node.js built-ins - no external dependencies needed!
 * 
 * Usage: node mock-api.js
 * Then access: http://localhost:3001/
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn, execSync } = require('child_process');

const PORT = 3001;
const startTime = Date.now();

// Load mock data from JSON files
function loadMockData() {
  try {
    const mockConfig = JSON.parse(fs.readFileSync(path.join(__dirname, 'mock-config.json'), 'utf8'));
    const mockDiagnostics = JSON.parse(fs.readFileSync(path.join(__dirname, 'mock-diagnostics.json'), 'utf8'));
    const mockPrinterDiscovery = JSON.parse(fs.readFileSync(path.join(__dirname, 'mock-printer-discovery.json'), 'utf8'));
    const mockNvsDump = JSON.parse(fs.readFileSync(path.join(__dirname, 'mock-nvs-dump.json'), 'utf8'));
    const mockWifiScan = JSON.parse(fs.readFileSync(path.join(__dirname, 'mock-wifi-scan.json'), 'utf8'));
    
    return { mockConfig, mockDiagnostics, mockPrinterDiscovery, mockNvsDump, mockWifiScan };
  } catch (error) {
    console.error('❌ Error loading mock data files:', error.message);
    process.exit(1);
  }
}

let { mockConfig, mockDiagnostics, mockPrinterDiscovery, mockNvsDump, mockWifiScan } = loadMockData();

// MIME types
const mimeTypes = {
  '.html': 'text/html',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.txt': 'text/plain',
  '.webmanifest': 'application/manifest+json'
};

function formatUptime(ms) {
  const seconds = Math.floor(ms / 1000);
  const minutes = Math.floor(seconds / 60);
  const hours = Math.floor(minutes / 60);
  
  if (hours > 0) {
    return `${hours}h ${minutes % 60}m ${seconds % 60}s`;
  } else if (minutes > 0) {
    return `${minutes}m ${seconds % 60}s`;
  } else {
    return `${seconds}s`;
  }
}

function sendJSON(res, data, statusCode = 200) {
  res.writeHead(statusCode, {
    'Content-Type': 'application/json',
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': 'GET, POST, PUT, DELETE, OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type, Authorization'
  });
  res.end(JSON.stringify(data, null, 2));
}

function serveFile(res, filePath) {
  const ext = path.extname(filePath).toLowerCase();
  const contentType = mimeTypes[ext] || 'application/octet-stream';
  
  fs.readFile(filePath, (err, data) => {
    if (err) {
      res.writeHead(404, {
        'Content-Type': 'text/html',
        'Access-Control-Allow-Origin': '*'
      });
      res.end('<h1>404 Not Found</h1>');
      return;
    }
    
    res.writeHead(200, {
      'Content-Type': contentType,
      'Access-Control-Allow-Origin': '*'
    });
    res.end(data);
  });
}

const server = http.createServer((req, res) => {
  const reqUrl = new URL(req.url, `http://${req.headers.host}`);
  const pathname = reqUrl.pathname;
  
  console.log(`${new Date().toISOString()} ${req.method} ${pathname}`);
  
  // Handle CORS preflight
  if (req.method === 'OPTIONS') {
    res.writeHead(200, {
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Methods': 'GET, POST, PUT, DELETE, OPTIONS',
      'Access-Control-Allow-Headers': 'Content-Type, Authorization'
    });
    res.end();
    return;
  }
  
  // Handle SSE endpoints that don't have /api/ prefix
  if (pathname === '/events') {
    // Server-Sent Events for general events
    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      'Connection': 'keep-alive',
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Headers': 'Cache-Control'
    });
    
    console.log('🔌 SSE: Events connection established');
    
    // Send initial printer discovery data with correct event type
    const printerDiscoveryData = { ...mockPrinterDiscovery };
    
    res.write(`event: printer-update\ndata: ${JSON.stringify(printerDiscoveryData)}\n\n`);
    
    const interval = setInterval(() => {
      if (res.destroyed) {
        clearInterval(interval);
        return;
      }
      // Send updated printer discovery data periodically with correct event type
      printerDiscoveryData.discovered_printers[0].last_power_on = new Date().toISOString();
      res.write(`event: printer-update\ndata: ${JSON.stringify(printerDiscoveryData)}\n\n`);
    }, 30000);
    
    req.on('close', () => {
      console.log('🔌 SSE: Events connection closed');
      clearInterval(interval);
    });
    return;
  }
  
  // API Routes
  if (pathname.startsWith('/api/')) {
    if (pathname === '/api/config' && req.method === 'GET') {
      setTimeout(() => sendJSON(res, mockConfig), 200);
      
    } else if (pathname === '/api/config' && req.method === 'POST') {
      let body = '';
      req.on('data', chunk => body += chunk);
      req.on('end', () => {
        console.log('📝 Config update:', body.substring(0, 100) + '...');
        setTimeout(() => {
          sendJSON(res, { 
            success: true, 
            message: "Configuration saved successfully" 
          });
        }, 500);
      });
      
    } else if (pathname === '/api/diagnostics') {
      // Update live data to match real ESP32 behavior
      mockDiagnostics.microcontroller.uptime_ms = Date.now() - startTime;
      mockDiagnostics.microcontroller.memory.free_heap = 114024 + Math.floor(Math.random() * 10000 - 5000);
      mockDiagnostics.microcontroller.memory.used_heap = mockDiagnostics.microcontroller.memory.total_heap - mockDiagnostics.microcontroller.memory.free_heap;
      mockDiagnostics.microcontroller.temperature = 40.5 + Math.random() * 5; // Simulate temperature variation
      
      setTimeout(() => sendJSON(res, mockDiagnostics), 150);
      
    } else if (pathname === '/api/nvs-dump') {
      // Update timestamp to current time
      const now = new Date();
      const timestamp = now.toLocaleDateString('en-GB', { 
        weekday: 'short', 
        day: '2-digit', 
        month: 'short', 
        year: 'numeric',
        hour: '2-digit',
        minute: '2-digit'
      });
      
      const nvsDumpResponse = {
        ...mockNvsDump,
        timestamp: timestamp
      };
      
      setTimeout(() => sendJSON(res, nvsDumpResponse), 200);
      
    } else if (pathname === '/api/status') {
      setTimeout(() => {
        sendJSON(res, {
          status: "ready",
          message: "Mock server running",
          timestamp: new Date().toISOString(),
          loading: false,
          error: null
        });
      }, 50);
      
    } else if (pathname === '/api/print' && req.method === 'POST') {
      let body = '';
      req.on('data', chunk => body += chunk);
      req.on('end', () => {
        console.log('🖨️  Print request received');
        setTimeout(() => {
          sendJSON(res, {
            success: true,
            message: "Print job completed successfully",
            characterCount: Math.floor(Math.random() * 200) + 50
          });
        }, 800);
      });
      
    } else if (pathname === '/api/led-effect' && req.method === 'POST') {
      let body = '';
      req.on('data', chunk => body += chunk);
      req.on('end', () => {
        console.log('💡 LED effect triggered');
        setTimeout(() => {
          sendJSON(res, {
            success: true,
            message: "LED effect started successfully"
          });
        }, 300);
      });
      
    } else if (pathname === '/api/scan-wifi') {
      console.log('📶 WiFi scan requested');
      setTimeout(() => sendJSON(res, mockWifiScan), 800);
      
    } else {
      sendJSON(res, { error: 'API endpoint not found' }, 404);
    }
    
  // Static file serving
  } else {
    let filePath;
    
    if (pathname === '/') {
      filePath = path.join(__dirname, '..', 'data', 'html', 'index.html');
    } else if (pathname === '/site.webmanifest') {
      // Special handling for manifest file
      filePath = path.join(__dirname, '..', 'data', 'favicon', 'site.webmanifest');
    } else if (pathname === '/favicon.ico' || 
               pathname === '/favicon.svg' || 
               pathname === '/favicon-96x96.png' || 
               pathname === '/apple-touch-icon.png') {
      // Special handling for favicon files at root level
      filePath = path.join(__dirname, '..', 'data', 'favicon', pathname.substring(1));
    } else if (pathname.startsWith('/html/') || 
               pathname.startsWith('/css/') || 
               pathname.startsWith('/js/') || 
               pathname.startsWith('/images/') ||
               pathname.startsWith('/favicon/')) {
      filePath = path.join(__dirname, '..', 'data', pathname.substring(1));
    } else if (pathname.endsWith('.html')) {
      // Handle HTML files directly (like /diagnostics.html)
      filePath = path.join(__dirname, '..', 'data', 'html', pathname.substring(1));
    } else {
      filePath = path.join(__dirname, '..', 'data', 'html', '404.html');
    }
    
    serveFile(res, filePath);
  }
});

server.listen(PORT, () => {
  console.log('\n🚀 Scribe Evolution Mock Server');
  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
  console.log(`🌐 Server running at: http://localhost:${PORT}`);
  console.log(`📱 Main Interface:    http://localhost:${PORT}/`);
  console.log(`🔧 Settings Page:     http://localhost:${PORT}/html/settings.html`);
  console.log(`📊 Diagnostics:       http://localhost:${PORT}/html/diagnostics.html`);
  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
  console.log('✨ Features:');
  console.log('   • Root URL serves main interface');
  console.log('   • All API endpoints with realistic delays');
  console.log('   • Mock data matching ESP32 responses');
  console.log('   • Static file serving (HTML, CSS, JS, images)');
  console.log('   • CORS enabled for local development');
  console.log('   • Live data updates (uptime, memory)');
  console.log('⏹️  Press Ctrl+C to stop');
  console.log('🔄 Press "r" + Enter to restart (picks up HTML/CSS/JS changes)');
  console.log('📄 Press "d" + Enter to reload JSON data files\n');
});

// Enable stdin for keyboard input
process.stdin.setRawMode(false);
process.stdin.resume();
process.stdin.setEncoding('utf8');

process.stdin.on('data', (key) => {
  const input = key.toString().trim().toLowerCase();
  
  if (input === 'r') {
    console.log('\n🔄 Restarting server...');
    console.log('📝 This will pick up HTML/CSS/JS changes');
    
    // Force close all connections immediately
    if (server.closeAllConnections) {
      server.closeAllConnections();
    }
    
    // Set a timeout to force the restart if server.close() hangs
    const forceRestartTimer = setTimeout(() => {
      console.log('⚠️  Force restarting...');
      
      // Spawn new process - keep it attached to terminal
      const newProcess = spawn(process.argv[0], [path.basename(__filename)], {
        cwd: __dirname,
        stdio: 'inherit',
        detached: false  // Keep attached to terminal
      });
      
      // Don't unref - we want to keep the process attached
      process.exit(0);
    }, 2000);
    
    // Try graceful close first
    server.close(() => {
      clearTimeout(forceRestartTimer);
      console.log('✅ Port freed, starting new server...');
      
      // Now spawn the new process - keep it attached to terminal
      const newProcess = spawn(process.argv[0], [path.basename(__filename)], {
        cwd: __dirname,
        stdio: 'inherit',
        detached: false  // Keep attached to terminal
      });
      
      // Don't unref - we want to keep the process attached
      process.exit(0);
    });
    
  } else if (input === 'd') {
    console.log('\n📄 Reloading JSON data files...');
    try {
      const reloaded = loadMockData();
      mockConfig = reloaded.mockConfig;
      mockDiagnostics = reloaded.mockDiagnostics;
      mockPrinterDiscovery = reloaded.mockPrinterDiscovery;
      mockNvsDump = reloaded.mockNvsDump;
      mockWifiScan = reloaded.mockWifiScan;
      console.log('✅ JSON data reloaded successfully\n');
    } catch (error) {
      console.error('❌ Error reloading JSON data:', error.message);
    }
  } else if (input === 'q' || key === '\u0003') { // Ctrl+C
    process.emit('SIGINT');
  }
});

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('\n👋 Shutting down mock server...');
  
  // Force close all connections immediately
  if (server.closeAllConnections) {
    server.closeAllConnections();
  }
  
  // Set a timeout to force exit if graceful shutdown takes too long
  const forceExitTimer = setTimeout(() => {
    console.log('⚠️  Force exiting...');
    process.exit(1);
  }, 2000);
  
  server.close((err) => {
    clearTimeout(forceExitTimer);
    if (err) {
      console.error('❌ Error during shutdown:', err.message);
      process.exit(1);
    } else {
      console.log('✅ Server stopped gracefully');
      process.exit(0);
    }
  });
});
