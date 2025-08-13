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
    
    // Initialize empty PRINTERS array (populated via SSE discovery)
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
    
  } catch (error) {
    console.error('Failed to load config:', error);
  }
}

/**
 * Initialize real-time printer discovery using Server-Sent Events (SSE)
 * Replaces polling with instant updates when printer status changes
 */
function initializePrinterDiscovery() {
  console.log('🔌 Initializing real-time printer discovery (SSE)');
  
  let eventSource = null;

  function setupSSE() {
    // Close existing connection if any
    if (eventSource) {
      eventSource.close();
    }
    
    eventSource = new EventSource('/events');
    
    // Handle printer discovery updates
    eventSource.addEventListener('printer-update', function(event) {
      try {
        console.log('🖨️ Real-time printer update received');
        const data = JSON.parse(event.data);
        
        // Update the global PRINTERS array
        updatePrintersFromData(data);
        
        // Refresh any UI elements that depend on the printer list
        refreshPrinterUI();
      } catch (error) {
        console.error('Error parsing printer update:', error);
      }
    });
    
    // Handle system status notifications
    eventSource.addEventListener('system-status', function(event) {
      try {
        const data = JSON.parse(event.data);
        console.log(`📡 System status: ${data.status} - ${data.message}`);
        showSystemNotification(data.status, data.message);
      } catch (error) {
        console.error('Error parsing system status:', error);
      }
    });
    
    eventSource.onerror = function(event) {
      console.error('SSE connection error:', event);
      // Reconnect after 5 seconds
      setTimeout(() => {
        console.log('🔄 Attempting to reconnect SSE...');
        setupSSE();
      }, 5000);
    };
    
    eventSource.onopen = function(event) {
      console.log('✅ Real-time updates connected');
    };
  }

  // Load initial printer data from already loaded config
  function loadInitialPrinterData() {
    console.log('📊 Setting up initial printer data from loaded config...');
    // Use already loaded config data instead of making another API call
    if (window.GLOBAL_CONFIG && window.GLOBAL_CONFIG.printer) {
      const localPrinter = {
        printer_id: 'local', // Use 'local' as identifier for local printer
        name: window.GLOBAL_CONFIG.printer.name,
        type: window.GLOBAL_CONFIG.printer.type,
        topic: window.GLOBAL_CONFIG.printer.topic,
        status: 'online' // Local printer is always online
      };
      PRINTERS.length = 0; // Clear existing
      PRINTERS.push(localPrinter); // Add local printer first
      refreshPrinterUI();
      console.log('✅ Initial printer data set up from config');
    } else {
      console.log('⚠️ No local printer config found');
    }
  }

  // Initialize: set up data first, then start real-time updates
  loadInitialPrinterData();
  setupSSE();

  // Clean up SSE connection when page unloads
  window.addEventListener('beforeunload', function() {
    if (eventSource) {
      eventSource.close();
    }
  });
}

/**
 * Update the global PRINTERS array from SSE printer update data
 */
function updatePrintersFromData(printerData) {
  if (printerData && printerData.discovered_printers) {
    // SSE sends all discovered printers (including local via MQTT discovery)
    PRINTERS.length = 0; // Clear existing
    PRINTERS.push(...printerData.discovered_printers); // Add all discovered printers
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

/**
 * Show subtle system notifications for connection status
 */
function showSystemNotification(status, message) {
  // Only show connection-related notifications to avoid spam
  if (!['connected', 'error', 'reconnecting'].includes(status)) {
    return;
  }
  
  // Create notification element
  const notification = document.createElement('div');
  notification.className = `notification notification-${status}`;
  notification.style.cssText = `
    position: fixed;
    top: 20px;
    right: 20px;
    padding: 10px 15px;
    border-radius: 4px;
    color: white;
    font-size: 14px;
    z-index: 10000;
    opacity: 0;
    transition: opacity 0.3s ease;
  `;
  
  // Set colors based on status
  switch (status) {
    case 'connected':
      notification.style.backgroundColor = '#10b981'; // green
      break;
    case 'error':
      notification.style.backgroundColor = '#ef4444'; // red
      break;
    case 'reconnecting':
      notification.style.backgroundColor = '#f59e0b'; // amber
      break;
  }
  
  notification.textContent = message;
  document.body.appendChild(notification);
  
  // Fade in
  requestAnimationFrame(() => {
    notification.style.opacity = '1';
  });
  
  // Fade out and remove after 3 seconds
  setTimeout(() => {
    notification.style.opacity = '0';
    setTimeout(() => {
      if (notification.parentNode) {
        notification.parentNode.removeChild(notification);
      }
    }, 300);
  }, 3000);
}
