/**
 * @file config.js
 * @brief Simple configuration utilities - no global state
 */

// Legacy variables for backward compatibility (will be set by individual pages as needed)
let MAX_CHARS = 1000; // Default fallback
let MAX_PROMPT_CHARS = 500; // Default fallback
let PRINTERS = []; // Will store all available printers

// Default prompts - keep in sync with C++ constants
const DEFAULT_MOTIVATION_PROMPT = "Generate a short, inspiring quote about creativity, technology, or daily life. Keep it under 200 characters.";

/**
 * Simple utility function to load configuration from server
 * Each page should call this directly when it needs config data
 */
async function loadConfigForPage() {
  try {
    const response = await fetch('/api/config');
    if (!response.ok) {
      throw new Error(`Config API returned ${response.status}: ${response.statusText}`);
    }
    const config = await response.json();
    
    // Set legacy variables for backward compatibility
    MAX_CHARS = config.validation?.maxCharacters || 1000;
    MAX_PROMPT_CHARS = 500; // Default from C++ config
    
    return config;
  } catch (error) {
    console.error('Failed to load config:', error);
    throw error;
  }
}

/**
 * Initialize real-time printer discovery using Server-Sent Events (SSE)
 * This is independent of configuration loading
 */
function initializePrinterDiscovery() {
  console.log('🔌 Initializing real-time printer discovery (SSE)');
  
  let eventSource = null;

  function setupSSE() {
    if (eventSource) {
      eventSource.close();
    }
    
    eventSource = new EventSource('/events');
    
    eventSource.addEventListener('printer-update', function(event) {
      try {
        console.log('🖨️ Real-time printer update received');
        const data = JSON.parse(event.data);
        updatePrintersFromData(data);
        refreshPrinterUI();
      } catch (error) {
        console.error('Error parsing printer update:', error);
      }
    });
    
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
      setTimeout(() => {
        console.log('🔄 Attempting to reconnect SSE...');
        setupSSE();
      }, 5000);
    };
    
    eventSource.onopen = function(event) {
      console.log('✅ Real-time updates connected');
    };
  }

  setupSSE();

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

// No automatic initialization - each page handles its own config loading
