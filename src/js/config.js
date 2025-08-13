/**
 * @file config.js
 * @brief Configuration management and global variables
 */

// Global configuration object - available to all scripts
window.GLOBAL_CONFIG = {};

// Global variables - will be set by the server
let MAX_CHARS; // Will be set by config endpoint - no default to ensure server provides it
let MAX_PROMPT_CHARS; // Will be set by config endpoint - no default to ensure server provides it
let PRINTERS = []; // Will store all available printers

// ETag tracking for efficient polling
let lastETag = null;

// Default prompts - keep in sync with C++ constants
const DEFAULT_MOTIVATION_PROMPT = "Generate a short, encouraging motivational message to help me stay focused and positive. Keep it brief, uplifting, and practical.";

/**
 * Load configuration from server and populate UI elements
 */
async function loadConfig() {
  try {
    const response = await fetch('/api/config');
    const config = await response.json();
    
    // Store configuration globally for other scripts to use
    window.GLOBAL_CONFIG = config;
    
    // Set legacy variables for backward compatibility
    MAX_CHARS = config.validation?.maxCharacters || 1000;
    MAX_PROMPT_CHARS = 500; // Default from C++ config
    
    // Initialize empty PRINTERS array (populated via discovery polling)
    PRINTERS = [];
    
    // Initialize printer selection UI (if function exists)
    if (typeof initializeConfigDependentUI === 'function') {
      initializeConfigDependentUI();
    }
    
    // Update character counters (only if elements exist - index page only)
    const messageTextarea = document.getElementById('message-textarea');
    const messageCounter = document.getElementById('char-counter');
    if (messageTextarea && messageCounter) {
      updateCharacterCount('message-textarea', 'char-counter', MAX_CHARS);
    }
    
    const customPromptTextarea = document.getElementById('custom-prompt');
    const customPromptCounter = document.getElementById('prompt-char-count');
    if (customPromptTextarea && customPromptCounter) {
      updateCharacterCount('custom-prompt', 'prompt-char-count', MAX_PROMPT_CHARS);
    }
    
    // Trigger event for other scripts that might be waiting for config
    window.dispatchEvent(new CustomEvent('configLoaded', { detail: config }));
    
    // Initialize Server-Sent Events for real-time printer discovery
    initializePrinterUpdates();
    
  } catch (error) {
    console.error('Failed to load config:', error);
  }
}

/**
 * Initialize Server-Sent Events for real-time printer discovery updates
 * Replaces polling with efficient real-time communication
 */
function initializePrinterUpdates() {
  // Check if EventSource is supported
  if (typeof(EventSource) === "undefined") {
    console.warn('⚠️ EventSource not supported, falling back to polling');
    initializePrinterPolling();
    return;
  }
  
  let reconnectDelay = 1000; // Start with 1 second
  const maxReconnectDelay = 30000; // Max 30 seconds
  let eventSource = null;
  
  function connectSSE() {
    console.log('📡 Connecting to Server-Sent Events...');
    
    eventSource = new EventSource('/events');
    
    eventSource.onopen = function(event) {
      console.log('✅ SSE connected successfully');
      reconnectDelay = 1000; // Reset delay on successful connection
    };
    
    eventSource.addEventListener('printer-update', function(event) {
      console.log('🖨️ Received printer update via SSE');
      
      try {
        const data = JSON.parse(event.data);
        
        // Update the global PRINTERS array
        updatePrintersFromData(data);
        
        // Refresh any UI elements that depend on the printer list
        refreshPrinterUI();
      } catch (error) {
        console.error('Failed to parse printer update:', error);
      }
    });
    
    eventSource.addEventListener('system-status', function(event) {
      console.log('📋 Received system status via SSE');
      
      try {
        const data = JSON.parse(event.data);
        console.log(`${data.level}: ${data.message}`);
        
        // You could show toast notifications here for system status
        if (data.level === 'error') {
          console.error('System error:', data.message);
        }
      } catch (error) {
        console.error('Failed to parse system status:', error);
      }
    });
    
    eventSource.onerror = function(event) {
      console.warn('❌ SSE connection error, will attempt to reconnect');
      eventSource.close();
      
      // Exponential backoff for reconnection
      setTimeout(connectSSE, reconnectDelay);
      reconnectDelay = Math.min(reconnectDelay * 2, maxReconnectDelay);
    };
  }
  
  // Start the SSE connection
  connectSSE();
  
  console.log('✅ Server-Sent Events initialized for real-time printer updates');
}

/**
 * Fallback polling implementation for browsers that don't support SSE
 * Uses the original polling logic with ETag optimization
 */
function initializePrinterPolling() {
  // Get polling interval from already loaded configuration
  function getPollingInterval() {
    const config = window.GLOBAL_CONFIG;
    const interval = config?.webInterface?.printerDiscoveryPollingInterval || 10000;
    console.log(`📡 Printer polling interval: ${interval/1000}s (fallback mode)`);
    return Promise.resolve(interval);
  }
  
  function pollForUpdates() {
    // Build request headers with ETag if we have one
    const headers = {};
    if (lastETag) {
      headers['If-None-Match'] = lastETag;
    }
    
    fetch('/api/printer-discovery', { headers })
      .then(response => {
        // Check for 304 Not Modified
        if (response.status === 304) {
          console.log('📡 Printer data unchanged (304)');
          return null; // No need to process response body
        }
        
        // Update ETag from response
        const etag = response.headers.get('ETag');
        if (etag) {
          lastETag = etag;
        }
        
        return response.json();
      })
      .then(data => {
        // Handle 304 responses (data will be null)
        if (!data) return;
        
        // Data has changed, update everything
        console.log('🖨️ Printer list updated');
        
        // Update the global PRINTERS array (data is now the direct response)
        updatePrintersFromData(data);
        
        // Refresh any UI elements that depend on the printer list
        refreshPrinterUI();
      })
      .catch(error => {
        console.warn('Failed to check for printer updates:', error);
      });
  }
  
  // Start immediate check, then set up polling with configured interval
  pollForUpdates();
  
  getPollingInterval().then(interval => {
    setInterval(pollForUpdates, interval);
    console.log(`✅ Smart printer polling initialized (${interval/1000}s updates) - fallback mode`);
  });
}

/**
 * Update the global PRINTERS array from API data
 */
function updatePrintersFromData(printerData) {
  if (printerData && printerData.discovered_printers) {
    PRINTERS.length = 0; // Clear existing
    PRINTERS.push(...printerData.discovered_printers);
  }
}

/**
 * Refresh UI elements that show printer information
 */
function refreshPrinterUI() {
  // Refresh printer dropdown if it exists
  if (typeof populatePrinterDropdown === 'function') {
    populatePrinterDropdown();
  }
  
  // Trigger custom event for other components
  const event = new CustomEvent('printersUpdated', { 
    detail: { printers: PRINTERS } 
  });
  document.dispatchEvent(event);
}
