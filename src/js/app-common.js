/**
 * @file shared.js
 * @brief Essential shared utilities for all pages
 */

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

// Make goBack available globally
window.goBack = goBack;
