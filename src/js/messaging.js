/**
 * @file messaging.js
 * @brief Message sending functions and quick actions
 */

/**
 * Unified form submission handler
 */
function handleSubmit(event) {
  event.preventDefault();
  
  const formData = new FormData(event.target);
  const printerTarget = formData.get('printer-target');
  const message = formData.get('message');
  
  if (!message.trim()) {
    alert('Your Scribe is waiting for something to write about');
    return;
  }
  
  // Check character limit
  if (message.length > MAX_CHARS) {
    alert(`Your message is a bit too chatty for your Scribe. Keep it under ${MAX_CHARS} characters (currently ${message.length} characters)`);
    return;
  }
  
  // Use the same 2-step pattern as quick actions
  sendUserMessage(printerTarget, message);
}

/**
 * Send user message using 2-step pattern (same as quick actions)
 */
function sendUserMessage(printerTarget, message) {
  // Get action config for display
  const config = getActionConfig('user-message');
  
  // Show confetti immediately
  triggerConfetti();

  // Step 1: Fetch content from user-message endpoint (adds MESSAGE prefix)
  fetch('/api/user-message', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ message: message, target: printerTarget })
  })
  .then(response => {
    if (response.ok) {
      return response.json();
    }
    throw new Error(`HTTP ${response.status}: ${response.statusText}`);
  })
  .then(contentResult => {
    if (!contentResult.content) {
      throw new Error('No content received from server');
    }
    
    // Step 2: Determine delivery endpoint based on printer target
    let deliveryEndpoint;
    let deliveryPayload;
    
    if (printerTarget === 'local-direct') {
      deliveryEndpoint = '/api/print-local';
      deliveryPayload = { message: contentResult.content };
    } else {
      deliveryEndpoint = '/api/mqtt-send';
      deliveryPayload = { 
        topic: printerTarget,
        message: contentResult.content
      };
    }
    
    // Step 3: Send content to delivery endpoint
    return fetch(deliveryEndpoint, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(deliveryPayload)
    });
  })
  .then(response => {
    if (response.ok) {
      return response.json();
    }
    // For error responses, try to parse JSON to get better error messages
    return response.json().then(errorData => {
      const errorMessage = errorData.error || errorData.message || `HTTP ${response.status}: ${response.statusText}`;
      throw new Error(errorMessage);
    }).catch(parseError => {
      // If JSON parsing fails, fall back to status text
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    });
  })
  .then(result => {
    if (result.success || result.status === 'success') {
      // Clear the message text box for next input
      const messageTextbox = document.getElementById('message-textarea') || document.querySelector('textarea[name="message"]');
      if (messageTextbox) {
        messageTextbox.value = '';
        // Update character counter after clearing
        updateCharacterCount('message-textarea', 'char-counter', MAX_CHARS);
      }
      
      // Show success message in a toast/notification instead of full screen
      showSuccessToast(`${config.name} scribed`);
    } else {
      throw new Error(result.error || result.message || 'Unknown error occurred');
    }
  })
  .catch(error => {
    console.error(`Failed to send ${config.name.toLowerCase()}:`, error);
    alert(`Your Scribe had trouble with that ${config.name.toLowerCase()}. Mind giving it another try?\n\nWhat went wrong: ${error.message}`);
  });
}

/**
 * Helper function to get action colors and name
 */
function getActionConfig(action) {
  switch (action) {
    case 'riddle':
      return { colors: ['#f59e0b', '#d97706', '#fbbf24'], name: 'Riddle' }; // Amber
    case 'joke':
      return { colors: ['#10b981', '#059669', '#34d399'], name: 'Joke' }; // Emerald
    case 'quote':
      return { colors: ['#8b5cf6', '#7c3aed', '#a78bfa'], name: 'Quote' }; // Purple
    case 'quiz':
      return { colors: ['#f59e0b', '#d97706', '#fbbf24'], name: 'Quiz' }; // Amber
    case 'news':
      return { colors: ['#f97316', '#ea580c', '#fb923c'], name: 'News' }; // Orange
    case 'poke':
      return { colors: ['#ec4899', '#be185d', '#f472b6'], name: 'Poke' }; // Pink
    case 'print-test':
      return { colors: ['#6b7280', '#4b5563', '#9ca3af'], name: 'Print Test' }; // Gray
    case 'user-message':
      return { colors: ['#3b82f6', '#1d4ed8', '#60a5fa'], name: 'Message' }; // Blue
    case 'scribe-message':
      return { colors: ['#8b5cf6', '#7c3aed', '#a78bfa'], name: 'Message' }; // Purple (legacy)
    default:
      return { colors: ['#6b7280', '#4b5563', '#9ca3af'], name: 'Unknown' }; // Gray
  }
}

/**
 * Handle quick actions (riddle, joke, quote, quiz, test print, etc.)
 */
function sendQuickAction(action) {
  const printerTarget = document.getElementById('printer-target').value;
  
  // Map action to content endpoint
  let contentEndpoint;
  switch (action) {
    case 'riddle':
      contentEndpoint = '/api/riddle';
      break;
    case 'joke':
      contentEndpoint = '/api/joke';
      break;
    case 'quote':
      contentEndpoint = '/api/quote';
      break;
    case 'quiz':
      contentEndpoint = '/api/quiz';
      break;
    case 'news':
      contentEndpoint = '/api/news';
      break;
    case 'poke':
      contentEndpoint = '/api/poke';
      break;
    case 'print-test':
      contentEndpoint = '/api/print-test';
      break;
    default:
      console.error('Unknown action:', action);
      alert('Your Scribe is confused by this action: ' + action);
      return;
  }

  // Get action config for display
  const config = getActionConfig(action);
  
  // Show confetti immediately
  triggerConfetti();

  // Step 1: Fetch content from content generation endpoint
  fetch(contentEndpoint, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ target: printerTarget }) // Pass target to content generation
  })
  .then(response => {
    if (response.ok) {
      return response.json();
    }
    throw new Error(`HTTP ${response.status}: ${response.statusText}`);
  })
  .then(contentResult => {
    if (!contentResult.content) {
      throw new Error('No content received from server');
    }
    
    // Step 2: Determine delivery endpoint based on printer target
    let deliveryEndpoint;
    let deliveryPayload;
    
    if (printerTarget === 'local-direct') {
      deliveryEndpoint = '/api/print-local';
      deliveryPayload = { message: contentResult.content };
    } else if (printerTarget === 'local-mqtt') {
      deliveryEndpoint = '/api/mqtt-send';
      deliveryPayload = { 
        topic: 'scribe/Pharkie/inbox', // Local device via MQTT
        message: contentResult.content
      };
    } else {
      deliveryEndpoint = '/api/mqtt-send';
      deliveryPayload = { 
        topic: printerTarget,
        message: contentResult.content
      };
    }
    
    // Step 3: Send content to delivery endpoint
    return fetch(deliveryEndpoint, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(deliveryPayload)
    });
  })
  .then(response => {
    if (response.ok) {
      return response.json();
    }
    // For error responses, try to parse JSON to get better error messages
    return response.json().then(errorData => {
      const errorMessage = errorData.error || errorData.message || `HTTP ${response.status}: ${response.statusText}`;
      throw new Error(errorMessage);
    }).catch(parseError => {
      // If JSON parsing fails, fall back to status text
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    });
  })
  .then(result => {
    if (result.success || result.status === 'success') {
      // Show success message in a toast/notification instead of full screen
      showSuccessToast(`${config.name} scribed`);
    } else {
      throw new Error(result.error || result.message || 'Unknown error occurred');
    }
  })
  .catch(error => {
    console.error(`Failed to send ${config.name.toLowerCase()}:`, error);
    alert(`Your Scribe stumbled on that ${config.name.toLowerCase()}. Want to try again?\n\nWhat happened: ${error.message}`);
  });
}

/**
 * Trigger confetti animation
 */
function triggerConfetti() {
  if (typeof confetti !== 'undefined') {
    // Detect if dark mode is active
    const isDarkMode = document.documentElement.classList.contains('dark') || 
                      window.matchMedia('(prefers-color-scheme: dark)').matches;
    
    // Choose colors that work well in both light and dark modes
    const colors = isDarkMode 
      ? ['#fbbf24', '#34d399', '#a78bfa', '#f472b6', '#fb7185'] // Bright yellows, greens, purples, pinks for dark mode
      : ['#3b82f6', '#ef4444', '#10b981', '#f59e0b', '#8b5cf6']; // Blues, reds, greens, oranges, purples for light mode
    
    confetti({
      particleCount: 100,
      spread: 70,
      origin: { y: 0.6 },
      colors: colors
    });
  }
}

/**
 * Show a success toast notification
 */
function showSuccessToast(message) {
  // Create toast element
  const toast = document.createElement('div');
  toast.className = 'fixed top-4 right-4 bg-green-500 dark:bg-green-600 text-white px-6 py-3 rounded-lg shadow-lg dark:shadow-2xl z-50 transform translate-x-full transition-transform duration-300';
  toast.textContent = message;
  
  document.body.appendChild(toast);
  
  // Animate in
  setTimeout(() => {
    toast.classList.remove('translate-x-full');
  }, 10);
  
  // Auto remove after 3 seconds
  setTimeout(() => {
    toast.classList.add('translate-x-full');
    setTimeout(() => {
      if (toast.parentNode) {
        toast.parentNode.removeChild(toast);
      }
    }, 300);
  }, 3000);
}
