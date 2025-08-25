/**
 * @file app-common.js
 * @brief Common functionality shared across all pages
 * @description Real-time printer discovery, system notifications, and shared UI functions
 */

/**
 * Initialize real-time printer discovery via Server-Sent Events (SSE)
 * Automatically reconnects on connection loss
 */
function initializePrinterDiscovery() {
    console.log('🔌 Initializing real-time printer discovery (SSE)');
    
    let eventSource = null;
    
    function connectSSE() {
        // Close existing connection if any
        if (eventSource) {
            eventSource.close();
        }
        
        // Create new SSE connection
        eventSource = new EventSource('/events');
        
        // Handle printer updates
        eventSource.addEventListener('printer-update', function(event) {
            try {
                console.log('🖨️ Real-time printer update received');
                const data = JSON.parse(event.data);
                updatePrintersFromData(data);
            } catch (error) {
                console.error('Error parsing printer update:', error);
            }
        });
        
        // Handle system status updates
        eventSource.addEventListener('system-status', function(event) {
            try {
                const data = JSON.parse(event.data);
                console.log(`📡 System status: ${data.status} - ${data.message}`);
                showSystemNotification(data.status, data.message);
            } catch (error) {
                console.error('Error parsing system status:', error);
            }
        });
        
        // Handle connection errors
        eventSource.onerror = function(error) {
            console.error('SSE connection error:', error);
            // Attempt to reconnect after 5 seconds
            setTimeout(() => {
                console.log('🔄 Attempting to reconnect SSE...');
                connectSSE();
            }, 5000);
        };
        
        // Handle successful connection
        eventSource.onopen = function(event) {
            console.log('✅ Real-time updates connected');
        };
    }
    
    // Start initial connection
    connectSSE();
    
    // Clean up on page unload
    window.addEventListener('beforeunload', function() {
        if (eventSource) {
            eventSource.close();
        }
    });
}

/**
 * Update printer data from SSE events
 * @param {Object} data - Printer data from server
 */
function updatePrintersFromData(data) {
    if (data && data.discovered_printers) {
        // Dispatch custom event for pages to listen to
        const event = new CustomEvent('printersUpdated', {
            detail: {
                printers: data.discovered_printers
            }
        });
        document.dispatchEvent(event);
    }
}

/**
 * Show system notification overlay
 * @param {string} status - Status type (connected, error, reconnecting)
 * @param {string} message - Notification message
 */
function showSystemNotification(status, message) {
    // Only show notifications for specific status types
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
    
    // Set background color based on status
    switch (status) {
        case 'connected':
            notification.style.backgroundColor = '#10b981'; // Green
            break;
        case 'error':
            notification.style.backgroundColor = '#ef4444'; // Red
            break;
        case 'reconnecting':
            notification.style.backgroundColor = '#f59e0b'; // Yellow
            break;
    }
    
    notification.textContent = message;
    document.body.appendChild(notification);
    
    // Fade in
    requestAnimationFrame(() => {
        notification.style.opacity = '1';
    });
    
    // Auto-remove after 3 seconds
    setTimeout(() => {
        notification.style.opacity = '0';
        setTimeout(() => {
            if (notification.parentNode) {
                notification.parentNode.removeChild(notification);
            }
        }, 300);
    }, 3000);
}

/**
 * Setup back button listeners for navigation
 */
function setupBackButtonListeners() {
    const backButton = document.getElementById('back-button');
    if (backButton) {
        backButton.addEventListener('click', goBack);
    }
}

/**
 * Navigate back to previous page or index
 */
function goBack() {
    // Check if there's browser history to go back to
    if (window.history.length > 1) {
        window.history.back();
    } else {
        // Fallback to index page
        window.location.href = '/';
    }
}

/**
 * Initialize common functionality on page load
 */
function initializeCommon() {
    // Initialize real-time printer discovery
    initializePrinterDiscovery();
    
    // Setup navigation listeners
    setupBackButtonListeners();
    
    console.log('🚀 Common functionality initialized');
}

// Auto-initialize when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeCommon);
} else {
    initializeCommon();
}

// Export functions for global use
window.initializePrinterDiscovery = initializePrinterDiscovery;
window.updatePrintersFromData = updatePrintersFromData;
window.showSystemNotification = showSystemNotification;
window.setupBackButtonListeners = setupBackButtonListeners;
window.goBack = goBack;