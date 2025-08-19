/**
 * @file config.js
 * @brief Simple configuration utilities - no global state
 */

/**
 * Initialize real-time printer discovery using Server-Sent Events (SSE)
 * This is independent of configuration loading and works with Alpine.js stores
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
 * Update printer data via Alpine.js stores
 */
function updatePrintersFromData(printerData) {
  if (printerData && printerData.discovered_printers) {
    // Dispatch event that Alpine.js stores can listen to
    const event = new CustomEvent('printersUpdated', { 
      detail: { printers: printerData.discovered_printers } 
    });
    document.dispatchEvent(event);
  }
}

/**
 * Refresh UI elements that show printer information
 * This is now handled by Alpine.js reactive stores
 */
function refreshPrinterUI() {
  // This is now automatically handled by Alpine.js reactivity
  // The updatePrintersFromData function dispatches events that stores listen to
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
