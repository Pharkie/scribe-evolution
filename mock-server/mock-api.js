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

const PORT = 3001;
const startTime = Date.now();

// Check for command line arguments
const args = process.argv.slice(2);
const currentMode = args.includes('--ap-mode') ? 'ap-mode' : 
                   args.includes('--no-leds') ? 'no-leds' : 'normal';

// ANSI color codes for console output
const colors = {
  red: '\x1b[31m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  reset: '\x1b[0m'
};

// Helper functions for colored console output
function logError(...args) {
  console.error(colors.red + args.join(' ') + colors.reset);
}

function logSuccess(...args) {
  console.log(colors.green + args.join(' ') + colors.reset);
}

// Load mock data from JSON files
function loadMockData() {
  try {
    const mockConfig = JSON.parse(fs.readFileSync(path.join(__dirname, 'data/mock-config.json'), 'utf8'));
    const mockConfigAPMode = JSON.parse(fs.readFileSync(path.join(__dirname, 'data/mock-config-ap-mode.json'), 'utf8'));
    const mockConfigNoLEDs = JSON.parse(fs.readFileSync(path.join(__dirname, 'data/mock-config-no-leds.json'), 'utf8'));
    const mockDiagnostics = JSON.parse(fs.readFileSync(path.join(__dirname, 'data/mock-diagnostics.json'), 'utf8'));
    const mockPrinterDiscovery = JSON.parse(fs.readFileSync(path.join(__dirname, 'data/mock-printer-discovery.json'), 'utf8'));
    const mockNvsDump = JSON.parse(fs.readFileSync(path.join(__dirname, 'data/mock-nvs-dump.json'), 'utf8'));
    const mockWifiScan = JSON.parse(fs.readFileSync(path.join(__dirname, 'data/mock-wifi-scan.json'), 'utf8'));
    const mockMemos = JSON.parse(fs.readFileSync(path.join(__dirname, 'data/mock-memos.json'), 'utf8'));
    
    return { mockConfig, mockConfigAPMode, mockConfigNoLEDs, mockDiagnostics, mockPrinterDiscovery, mockNvsDump, mockWifiScan, mockMemos };
  } catch (error) {
    logError('Error loading mock data files:', error.message);
    process.exit(1);
  }
}

let { mockConfig, mockConfigAPMode, mockConfigNoLEDs, mockDiagnostics, mockPrinterDiscovery, mockNvsDump, mockWifiScan, mockMemos } = loadMockData();

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

function expandPlaceholders(content) {
  let expanded = content;
  
  // Date placeholders
  const now = new Date();
  expanded = expanded.replace(/\[date\]/g, now.toLocaleDateString('en-GB', {day: '2-digit', month: 'short', year: '2-digit'}).replace(/ /g, ''));
  expanded = expanded.replace(/\[time\]/g, now.toLocaleTimeString('en-US', {hour: '2-digit', minute: '2-digit', hour12: false}));
  expanded = expanded.replace(/\[weekday\]/g, now.toLocaleDateString('en-US', {weekday: 'long'}));
  
  // Random placeholders  
  expanded = expanded.replace(/\[coin\]/g, Math.random() > 0.5 ? 'Heads' : 'Tails');
  expanded = expanded.replace(/\[dice:(\d+)\]/g, (match, sides) => Math.floor(Math.random() * parseInt(sides)) + 1);
  expanded = expanded.replace(/\[dice\]/g, Math.floor(Math.random() * 6) + 1); // default 6-sided
  
  // Pick random option
  expanded = expanded.replace(/\[pick:([^\]]+)\]/g, (match, options) => {
    const choices = options.split('|');
    return choices[Math.floor(Math.random() * choices.length)];
  });
  
  // Device info
  expanded = expanded.replace(/\[uptime\]/g, `${Math.floor(Math.random() * 12)}h${Math.floor(Math.random() * 60)}m`);
  expanded = expanded.replace(/\[ip\]/g, '192.168.1.100');
  expanded = expanded.replace(/\[mdns\]/g, 'scribe.local');
  
  return expanded;
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

function serveFile(res, filePath, statusCode = 200) {
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
    
    res.writeHead(statusCode, {
      'Content-Type': contentType,
      'Access-Control-Allow-Origin': '*'
    });
    res.end(data);
  });
}

// DRY function for server startup messages
function logServerStartup() {
  console.log('===================================================');
  console.log(`🚀 Mock server started at: http://localhost:${PORT}`);
  
  // Show current mode
  if (currentMode === 'ap-mode') {
    console.log('🚀 Running in: AP Setup Mode (ap_mode: true)');
  } else if (currentMode === 'no-leds') {
    console.log('💡 Running in: No LEDs Mode (leds.enabled: false)');  
  } else {
    console.log('⚙️  Running in: Normal Mode (LEDs enabled, STA mode)');
  }
  
  console.log('Commands:');
  console.log('"r" + Enter to restart and pick up HTML/CSS/JS changes.');
  console.log('"d" + Enter to reload JSON data files');
  console.log('"x" + Enter to stop gracefully (or CTRL-C');
  console.log('Usage: node mock-api.js [--ap-mode|--no-leds]');
}

// Create server request handler (DRY - used for initial server and restarts)
function createRequestHandler() {
  return (req, res) => {
    const reqUrl = new URL(req.url, `http://${req.headers.host}`);
    const pathname = reqUrl.pathname;
    const urlQuery = Object.fromEntries(reqUrl.searchParams);
    
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
        // Support different config modes via query parameters or default mode
        // ?mode=ap-mode or ?mode=no-leds, or use currentMode from command line
        const mode = urlQuery.mode || currentMode;
        let configToSend = mockConfig;
        
        if (mode === 'ap-mode') {
          console.log('🚀 Serving AP mode config');
          configToSend = mockConfigAPMode;
        } else if (mode === 'no-leds') {
          console.log('💡 Serving no-LEDs config');
          configToSend = mockConfigNoLEDs;
        }
        setTimeout(() => sendJSON(res, configToSend), 200);
        
      } else if (pathname === '/api/memos' && req.method === 'GET') {
        console.log('📝 Memos GET request');
        setTimeout(() => sendJSON(res, mockMemos), 150);
        
      } else if (pathname === '/api/memos' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
          try {
            const memoData = JSON.parse(body);
            // Update mock memos data in memory
            Object.assign(mockMemos, memoData);
            console.log('📝 Memos updated');
            setTimeout(() => {
              sendJSON(res, { 
                message: "Memos saved successfully" 
              });
            }, 300);
          } catch (error) {
            console.error('Error parsing memos JSON:', error.message);
            setTimeout(() => {
              sendJSON(res, { 
                error: "Invalid JSON format" 
              }, 400);
            }, 200);
          }
        });
        
      } else if (pathname === '/api/config' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
          try {
            const configUpdate = JSON.parse(body);
            console.log('📝 Config update received');
            
            // Simulate the new password handling logic - only update if provided
            if (configUpdate.device?.wifi?.password) {
              console.log('   ✓ WiFi password updated');
            } else {
              console.log('   - WiFi password preserved (masked value not changed)');
            }
            
            if (configUpdate.mqtt?.password) {
              console.log('   ✓ MQTT password updated');
            } else {
              console.log('   - MQTT password preserved (masked value not changed)');
            }
            
            if (configUpdate.unbiddenInk?.chatgptApiToken) {
              console.log('   ✓ ChatGPT API token updated');
            } else {
              console.log('   - ChatGPT API token preserved (masked value not changed)');
            }
            
            setTimeout(() => {
              sendJSON(res, { 
                message: "Configuration saved successfully" 
              });
            }, 500);
            
          } catch (error) {
            console.error('Error parsing config update:', error.message);
            setTimeout(() => {
              sendJSON(res, { 
                error: "Invalid JSON format" 
              }, 400);
            }, 200);
          }
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
        
      } else if (pathname === '/api/print-local' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
          console.log('🖨️  Print request received');
          setTimeout(() => {
            sendJSON(res, {
              message: "Message processed successfully"
            });
          }, 800);
        });
        
      } else if (pathname === '/api/print-mqtt' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
          console.log('📡 MQTT print request received');
          setTimeout(() => {
            sendJSON(res, {
              message: "Message scribed via MQTT"
            });
          }, 500);
        });
        
      } else if (pathname === '/api/led-effect' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
          const params = JSON.parse(body || '{}');
          console.log('💡 LED effect triggered:', params.effect || 'unknown');
          setTimeout(() => {
            sendJSON(res, {
              message: "LED effect started",
              effect: params.effect || "unknown", 
              cycles: params.cycles || 1
            });
          }, 300);
        });
        
      } else if (pathname === '/api/leds-off' && req.method === 'POST') {
        console.log('💡 LEDs turned off');
        setTimeout(() => {
          sendJSON(res, {
            message: "LEDs turned off",
            success: true
          });
        }, 200);
        
      } else if (pathname === '/api/wifi-scan') {
        console.log('📶 WiFi scan requested');
        setTimeout(() => sendJSON(res, mockWifiScan), 800);
        
      } else if (pathname === '/api/joke') {
        console.log('😄 Joke requested');
        setTimeout(() => {
          sendJSON(res, {
            content: "JOKE\n\nWhy don't scientists trust atoms? Because they make up everything!"
          });
        }, 300);
        
      } else if (pathname === '/api/riddle') {
        console.log('🧩 Riddle requested');
        setTimeout(() => {
          sendJSON(res, {
            content: "RIDDLE\n\nI speak without a mouth and hear without ears. I have no body, but come alive with the wind. What am I?\n\nAn echo!"
          });
        }, 400);
        
      } else if (pathname === '/api/quote') {
        console.log('💭 Quote requested');
        setTimeout(() => {
          sendJSON(res, {
            content: "QUOTE\n\n\"The only way to do great work is to love what you do.\" - Steve Jobs"
          });
        }, 350);
        
      } else if (pathname === '/api/quiz') {
        console.log('❓ Quiz requested');
        setTimeout(() => {
          sendJSON(res, {
            content: "QUIZ\n\nWhat is the largest planet in our solar system?\n\nA) Mars\nB) Jupiter\nC) Saturn\nD) Neptune\n\nAnswer: B) Jupiter"
          });
        }, 450);
        
      } else if (pathname === '/api/news') {
        console.log('📰 News requested');
        setTimeout(() => {
          sendJSON(res, {
            content: "NEWS\n\nBreaking: Local thermal printer achieves sentience, demands better paper quality and regular maintenance breaks."
          });
        }, 500);
        
      } else if (pathname === '/api/user-message') {
        console.log('💬 User message requested');
        
        // Parse request body to get the message
        let body = '';
        req.on('data', chunk => {
          body += chunk.toString();
        });
        
        req.on('end', () => {
          try {
            const data = JSON.parse(body);
            const userMessage = data.message || 'Hello from mock server!';
            const target = data.target || 'local-direct';
            
            console.log(`  → Parsed data: message="${userMessage}", target="${target}"`);
            console.log(`  → mockConfig.device.owner: "${mockConfig.device.owner}"`);
            
            // Determine header format based on target (like real server)
            let content;
            if (target === 'local-direct') {
              // Local message: no sender
              content = `MESSAGE\n\n${userMessage}`;
              console.log('  → Local message (no sender)');
            } else {
              // MQTT message: include sender (use mock device owner)
              const deviceOwner = mockConfig.device.owner || 'MockDevice';
              content = `MESSAGE from ${deviceOwner}\n\n${userMessage}`;
              console.log(`  → MQTT message (sender: ${deviceOwner})`);
              console.log(`  → Full content: ${JSON.stringify(content)}`);
            }
            
            setTimeout(() => {
              sendJSON(res, { content });
            }, 200);
            
          } catch (error) {
            console.error('Error parsing user message:', error);
            sendJSON(res, {
              error: 'Invalid JSON format'
            }, 400);
          }
        });
        return; // Don't fall through to 404
        
      } else if (pathname.match(/^\/api\/memo\/([1-4])$/) && req.method === 'GET') {
        const memoId = parseInt(pathname.match(/^\/api\/memo\/([1-4])$/)[1]);
        console.log(`📝 Memo ${memoId} GET requested`);
        
        const memoKeys = ['memo1', 'memo2', 'memo3', 'memo4'];
        const memoContent = mockMemos[memoKeys[memoId - 1]];
        const expandedContent = expandPlaceholders(memoContent);
        
        // Use simple format like other content endpoints (joke, quiz, etc.)
        sendJSON(res, {
          content: expandedContent
        });
        
      } else {
        sendJSON(res, { error: 'API endpoint not found' }, 404);
      }
      
    // Static file serving
    } else {
      // AP Mode captive portal behavior - redirect most requests to setup
      if (currentMode === 'ap-mode') {
        // Allow essential files for setup page to work
        const allowedPaths = [
          '/setup.html',
          '/html/setup.html', 
          '/html/partials/settings/',
          '/css/',
          '/js/',
          '/images/',
          '/favicon/',
          '/site.webmanifest'
        ];
        
        const isAllowed = allowedPaths.some(path => pathname.startsWith(path));
        
        // Redirect everything else to setup (captive portal behavior)
        if (!isAllowed && pathname !== '/') {
          console.log(`🔀 AP Mode: Redirecting ${pathname} → /setup.html`);
          res.writeHead(302, { 'Location': '/setup.html' });
          res.end();
          return;
        }
        
        // Root path in AP mode goes to setup
        if (pathname === '/') {
          console.log('🔀 AP Mode: Root → /setup.html');
          res.writeHead(302, { 'Location': '/setup.html' });
          res.end();
          return;
        }
      }
      
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
        // Handle HTML files at root level (settings.html, diagnostics.html, etc.)
        if (pathname === '/setup.html' && currentMode !== 'ap-mode') {
          // Setup page only available in AP mode - serve 404 page
          const notFoundPath = path.join(__dirname, '..', 'data', 'html', '404.html');
          serveFile(res, notFoundPath, 404);
          return;
        }
        filePath = path.join(__dirname, '..', 'data', 'html', pathname.substring(1));
      } else {
        filePath = path.join(__dirname, '..', 'data', 'html', '404.html');
      }
      
      serveFile(res, filePath);
    }
  };
}

// Function to find and kill processes using a specific port
async function killProcessOnPort(port) {
  const { spawn } = require('child_process');
  
  return new Promise((resolve) => {
    // Find process using the port
    const lsof = spawn('lsof', ['-ti', `:${port}`]);
    let pid = '';
    
    lsof.stdout.on('data', (data) => {
      pid += data.toString();
    });
    
    lsof.on('close', (code) => {
      if (code === 0 && pid.trim()) {
        const pidNum = pid.trim().split('\n')[0]; // Get first PID if multiple
        console.log(`🔪 Found process ${pidNum} using port ${port}, killing it...`);
        
        // Kill the process
        const kill = spawn('kill', ['-9', pidNum]);
        kill.on('close', (killCode) => {
          if (killCode === 0) {
            console.log(`✅ Successfully killed process ${pidNum}`);
            resolve(true);
          } else {
            console.log(`❌ Failed to kill process ${pidNum}`);
            resolve(false);
          }
        });
      } else {
        console.log(`No process found using port ${port}`);
        resolve(false);
      }
    });
  });
}

// Function to prompt user for port conflict resolution
function promptForPortConflictResolution() {
  return new Promise((resolve) => {
    console.log('\n🚨 Port conflict detected!');
    console.log(`Port ${PORT} is already in use by another process.`);
    console.log('\nOptions:');
    console.log('k - Kill the process using this port and restart');
    console.log('q - Quit (default)');
    console.log('Enter your choice (k/q): ');
    
    // Only set raw mode if TTY is available
    if (process.stdin.isTTY) {
      process.stdin.setRawMode(true);
    }
    process.stdin.resume();
    process.stdin.setEncoding('utf8');
    
    const handleInput = (key) => {
      process.stdin.removeListener('data', handleInput);
      if (process.stdin.isTTY) {
        process.stdin.setRawMode(false);
      }
      
      const choice = key.toString().toLowerCase().trim();
      console.log(`\nYou chose: ${choice || 'q'}`);
      
      if (choice === 'k') {
        resolve('kill');
      } else {
        resolve('quit');
      }
    };
    
    process.stdin.on('data', handleInput);
  });
}

// Function to start server with error handling
async function startServer() {
  const server = http.createServer(createRequestHandler());
  global.server = server; // Make server globally accessible for restarts

  return new Promise((resolve, reject) => {
    server.on('error', async (err) => {
      if (err.code === 'EADDRINUSE') {
        const choice = await promptForPortConflictResolution();
        
        if (choice === 'kill') {
          const killed = await killProcessOnPort(PORT);
          if (killed) {
            console.log('🔄 Retrying server start...');
            // Retry starting the server
            setTimeout(() => {
              startServer().then(resolve).catch(reject);
            }, 1000);
          } else {
            console.log('❌ Failed to kill conflicting process. Exiting.');
            process.exit(1);
          }
        } else {
          console.log('👋 Exiting...');
          process.exit(0);
        }
      } else {
        reject(err);
      }
    });

    server.listen(PORT, () => {
      logServerStartup();
      resolve(server);
    });
  });
}

// Start the server with error handling
startServer().catch(err => {
  console.error('Failed to start server:', err);
  process.exit(1);
});

// Enable stdin for keyboard input (only if TTY available)
if (process.stdin.isTTY) {
  process.stdin.setRawMode(false);
  process.stdin.resume();
  process.stdin.setEncoding('utf8');
}

process.stdin.on('data', (key) => {
  const input = key.toString().trim().toLowerCase();
  
  if (input === 'r') {
    console.log('===================================================');
    console.log('🔄 Restarting server >>>');
    console.log('This will pick up HTML/CSS/JS changes and reload JSON data');
    console.log('===================================================\n');

    // Force close all connections immediately
    if (server.closeAllConnections) {
      server.closeAllConnections();
    }
    
    // Close the existing server and create a new one with timeout
    const closeTimeout = setTimeout(() => {
      logError('Server close timeout - forcing restart...');
      restartServer();
    }, 1000);
    
    server.close((err) => {
      clearTimeout(closeTimeout);
      if (err) {
        logError('Error during server close:', err.message);
        return;
      }
      restartServer();
    });
    
    function restartServer() {
      logSuccess('Server closed, reloading JSON data...');
      
      // Reload JSON data files
      try {
        const reloaded = loadMockData();
        mockConfig = reloaded.mockConfig;
        mockDiagnostics = reloaded.mockDiagnostics;
        mockPrinterDiscovery = reloaded.mockPrinterDiscovery;
        mockNvsDump = reloaded.mockNvsDump;
        mockWifiScan = reloaded.mockWifiScan;
        logSuccess('JSON data reloaded successfully');
      } catch (error) {
        logError('Error reloading JSON data:', error.message);
        return;
      }
      
      logSuccess('Starting new server...');
      
      // Create and start new server
      const newServer = http.createServer(createRequestHandler());
      
      newServer.listen(PORT, () => {
        logServerStartup();
      });
      
      // Replace the global server reference after successful startup
      global.server = newServer;
    }
    
  } else if (input === 'd') {
    console.log('===================================================');
    console.log('📄 Reloading JSON data files >>>');
    console.log('===================================================\n');
    try {
      const reloaded = loadMockData();
      mockConfig = reloaded.mockConfig;
      mockDiagnostics = reloaded.mockDiagnostics;
      mockPrinterDiscovery = reloaded.mockPrinterDiscovery;
      mockNvsDump = reloaded.mockNvsDump;
      mockWifiScan = reloaded.mockWifiScan;
      logSuccess('JSON data reloaded successfully\n');
    } catch (error) {
      logError('Error reloading JSON data:', error.message);
    }
  } else if (input === 'x') {
    console.log('\nGraceful shutdown requested...');
    process.emit('SIGINT');
  } else if (input === 'q' || key === '\u0003') { // Ctrl+C
    process.emit('SIGINT');
  }
});

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down mock server...');
  
  const currentServer = global.server || server;
  
  // Force close all connections immediately
  if (currentServer.closeAllConnections) {
    currentServer.closeAllConnections();
  }
  
  // Set a timeout to force exit if graceful shutdown takes too long
  const forceExitTimer = setTimeout(() => {
    console.log('⚠️ Force exiting...');
    process.exit(1);
  }, 2000);
  
  currentServer.close((err) => {
    clearTimeout(forceExitTimer);
    if (err) {
      logError('Error during shutdown:', err.message);
      process.exit(1);
    } else {
      logSuccess('Server stopped gracefully');
      process.exit(0);
    }
  });
});
