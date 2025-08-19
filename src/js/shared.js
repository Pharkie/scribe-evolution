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
  
  // Start Alpine.js after all stores are loaded
  if (window.startAlpine && typeof window.startAlpine === 'function') {
    window.startAlpine();
  }
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
 */
function handleTextareaKeydown(event) {
  // Enter to submit form (unless Shift is held)
  if (event.key === 'Enter' && !event.shiftKey) {
    event.preventDefault();
    const form = document.getElementById('printer-form');
    if (form) {
      // Create a proper submit event that won't cause double submission
      const submitEvent = new Event('submit', { 
        bubbles: false,  // Don't bubble to prevent duplicate handlers
        cancelable: true 
      });
      Object.defineProperty(submitEvent, 'target', {
        value: form,
        enumerable: true
      });
      handleSubmit(submitEvent);
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
  // Add hover effects for info icons
  const infoIcons = document.querySelectorAll('.info-icon');
  infoIcons.forEach(icon => {
    icon.addEventListener('mouseenter', function() {
      // Show tooltip logic here if needed
    });
  });
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
