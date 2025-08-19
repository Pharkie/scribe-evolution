/**
 * @file shared.js
 * @brief Shared functionality across all pages
 */

/**
 * Initialize shared functionality when DOM is loaded
 */
document.addEventListener('DOMContentLoaded', function() {
  // Add keyboard event listeners
  document.addEventListener('keydown', handleKeyPress);
  
  // Initialize basic UI elements
  initializeSharedUI();
  
  // Setup back button event listeners
  setupBackButtonListeners();
  
  // Start Alpine.js with a slight delay to ensure all page stores are loaded
  setTimeout(() => {
    if (window.startAlpine && typeof window.startAlpine === 'function') {
      console.log('🏔️ Starting Alpine.js...');
      window.startAlpine();
    } else {
      console.error('🏔️ Alpine.js startAlpine function not available');
    }
  }, 50);
});

/**
 * Setup back button event listeners
 */
function setupBackButtonListeners() {
  // Handle back buttons (used on 404 and diagnostics pages)
  const backButton = document.getElementById('back-button');
  if (backButton) {
    backButton.addEventListener('click', goBack);
  }
}

/**
 * Keyboard shortcut handling
 */
function handleKeyPress(event) {
  // Ctrl/Cmd + Enter to submit form
  if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
    event.preventDefault();
    const form = document.querySelector('form');
    if (form) {
      form.dispatchEvent(new Event('submit'));
    }
  }
  
  // Esc to close modals
  if (event.key === 'Escape') {
    closeDiagnostics();
    const settingsPanel = document.getElementById('settings-panel');
    if (settingsPanel && !settingsPanel.classList.contains('hidden')) {
      toggleSettings();
    }
  }
}

/**
 * Handle textarea keydown events for Enter/Shift+Enter behavior
 * This is for backward compatibility with any non-Alpine forms
 */
function handleTextareaKeydown(event) {
  // Enter to submit form (unless Shift is held)
  if (event.key === 'Enter' && !event.shiftKey) {
    event.preventDefault();
    const form = event.target.closest('form');
    if (form) {
      // Trigger form submission through Alpine.js if available
      const submitEvent = new Event('submit', { 
        bubbles: true,
        cancelable: true 
      });
      form.dispatchEvent(submitEvent);
    }
  }
  // Shift+Enter allows normal line break (default behavior)
}

/**
 * Initialize shared UI elements and state
 */
function initializeSharedUI() {
  // Hide loading states
  const loadingElements = document.querySelectorAll('.loading');
  loadingElements.forEach(el => el.classList.add('hidden'));
  
  // Initialize any tooltips or interactive elements
  initializeTooltips();
  
  // Check for any startup messages
  checkStartupMessages();
}

/**
 * Initialize tooltips and help text
 */
function initializeTooltips() {
  // Add hover effects for info icons if needed
  const infoIcons = document.querySelectorAll('.info-icon');
  if (infoIcons.length > 0) {
    // Currently no tooltip implementation needed
    // This function remains for future tooltip functionality
  }
}

/**
 * Check for any startup messages or notifications
 */
function checkStartupMessages() {
  // Check URL parameters for any messages
  const urlParams = new URLSearchParams(window.location.search);
  const message = urlParams.get('message');
  const type = urlParams.get('type');
  
  if (message) {
    if (type === 'success') {
      showSuccessMessage(decodeURIComponent(message));
    } else if (type === 'error') {
      showErrorMessage(decodeURIComponent(message));
    }
    
    // Clean up URL
    window.history.replaceState({}, document.title, window.location.pathname);
  }
}

/**
 * Show error message to user
 */
function showErrorMessage(message) {
  const container = document.getElementById('message-container') || document.body;
  
  const errorDiv = document.createElement('div');
  errorDiv.className = 'fixed top-4 right-4 bg-red-500 dark:bg-red-600 text-white px-4 py-2 rounded-lg shadow-lg dark:shadow-2xl z-50';
  errorDiv.textContent = message;
  
  container.appendChild(errorDiv);
  
  // Auto-remove after 5 seconds
  setTimeout(() => {
    if (errorDiv.parentNode) {
      errorDiv.parentNode.removeChild(errorDiv);
    }
  }, 5000);
}

/**
 * Show success message to user
 */
function showSuccessMessage(message) {
  const container = document.getElementById('message-container') || document.body;
  
  const successDiv = document.createElement('div');
  successDiv.className = 'fixed top-4 right-4 bg-green-500 dark:bg-green-600 text-white px-4 py-2 rounded-lg shadow-lg dark:shadow-2xl z-50';
  successDiv.textContent = message;
  
  container.appendChild(successDiv);
  
  // Auto-remove after 3 seconds
  setTimeout(() => {
    if (successDiv.parentNode) {
      successDiv.parentNode.removeChild(successDiv);
    }
  }, 3000);
}

/**
 * Show message to user with specific type
 */
function showMessage(message, type = 'info') {
  const container = document.getElementById('message-container') || document.body;
  
  let bgColor, duration;
  switch(type) {
    case 'success':
      bgColor = 'bg-green-500 dark:bg-green-600';
      duration = 3000;
      break;
    case 'error':
      bgColor = 'bg-red-500 dark:bg-red-600';
      duration = 5000;
      break;
    case 'warning':
      bgColor = 'bg-yellow-500 dark:bg-yellow-600';
      duration = 4000;
      break;
    case 'info':
    default:
      bgColor = 'bg-blue-500 dark:bg-blue-600';
      duration = 3000;
      break;
  }
  
  const messageDiv = document.createElement('div');
  messageDiv.className = `fixed top-4 right-4 ${bgColor} text-white px-4 py-2 rounded-lg shadow-lg dark:shadow-2xl z-50`;
  messageDiv.textContent = message;
  
  container.appendChild(messageDiv);
  
  // Auto-remove after duration
  setTimeout(() => {
    if (messageDiv.parentNode) {
      messageDiv.parentNode.removeChild(messageDiv);
    }
  }, duration);
}

// Make showMessage available globally
window.showMessage = showMessage;

/**
 * Navigate back to previous page or home
 */
function goBack() {
  // Check if there's a previous page in history
  if (window.history.length > 1 && document.referrer) {
    window.history.back();
  } else {
    // Default to home page if no history
    window.location.href = '/';
  }
}
