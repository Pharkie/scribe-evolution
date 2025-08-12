/**
 * @file index.js
 * @brief Index page specific functionality
 */

/**
 * Initialize index page when DOM is loaded
 */
document.addEventListener('DOMContentLoaded', function() {
  // Initialize index-specific UI
  initializeIndexUI();
  
  // Initialize printer selection
  initializeConfigDependentUI();
  
  // Listen for printer updates from the polling system
  document.addEventListener('printersUpdated', function(event) {
    console.log('🔄 Printers updated, refreshing index page printer selection');
    initializePrinterSelection();
  });
});

/**
 * Initialize index page specific UI elements
 */
function initializeIndexUI() {
  // Update character counter on input for main message textarea
  const messageInput = document.getElementById('message-textarea');
  if (messageInput) {
    messageInput.addEventListener('input', () => updateCharacterCount('message-textarea', 'char-counter', MAX_CHARS));
    // Set initial character counter
    updateCharacterCount('message-textarea', 'char-counter', MAX_CHARS);
  }
}

/**
 * Initialize printer selection UI with icons
 */
function initializePrinterSelection() {
  const container = document.getElementById('printer-selection');
  if (!container) return;
  
  // Clear existing content
  container.innerHTML = '';
  
  // Get local printer name from config
  const localPrinterName = window.GLOBAL_CONFIG?.printer?.name || 'Unknown';
  
  // Add local-direct printer option with dynamic name
  const localDirectOption = createPrinterOption('local-direct', '🖨️', `Local direct (${localPrinterName})`, true, null);
  container.appendChild(localDirectOption);
  
  // Add remote printers from PRINTERS config (this will now include discovered printers)
  if (typeof PRINTERS !== 'undefined') {
    PRINTERS.forEach(printer => {
      // Construct the MQTT topic from printer name
      const topic = `scribe/${printer.name}/print`;
      const option = createPrinterOption(topic, '📡', `${printer.name} via MQTT`, false, printer);
      container.appendChild(option);
    });
  }
}

/**
 * Create a printer option element
 */
function createPrinterOption(value, icon, name, isSelected = false, printerData = null) {
  const option = document.createElement('div');
  option.className = `printer-option cursor-pointer p-4 rounded-xl border-2 transition-all duration-200 hover:scale-105 hover:shadow-md ${
    isSelected ? 'border-orange-400 bg-orange-50 dark:bg-orange-900/20 text-orange-700 dark:text-orange-300 dark:border-orange-600' : 'border-gray-200 dark:border-gray-600 bg-gray-50 dark:bg-gray-700 text-gray-700 dark:text-gray-300 hover:border-gray-300 dark:hover:border-gray-500'
  }`;
  option.setAttribute('data-value', value);
  
  // Determine connection type and additional info for display
  let connectionType = 'Unknown';
  let additionalInfo = '';
  
  if (value === 'local-direct') {
    connectionType = 'Direct connection';
  } else if (value === 'local-mqtt') {
    connectionType = 'MQTT connection';
  } else {
    connectionType = 'MQTT connection';
    
    // Add additional info for discovered printers
    if (printerData) {
      const infoParts = [];
      if (printerData.ip_address) {
        infoParts.push(`IP: ${printerData.ip_address}`);
      }
      if (printerData.firmware_version) {
        infoParts.push(`v${printerData.firmware_version}`);
      }
      if (printerData.last_seen !== undefined) {
        const seenText = printerData.last_seen === 0 ? 'Just now' : `${printerData.last_seen}s ago`;
        infoParts.push(`Seen: ${seenText}`);
      }
      if (infoParts.length > 0) {
        additionalInfo = ` • ${infoParts.join(' • ')}`;
      }
    }
  }
  
  option.innerHTML = `
    <div class="flex items-center space-x-3">
      <span class="text-2xl">${icon}</span>
      <div class="flex-1 min-w-0">
        <div class="font-medium text-sm truncate">${name}</div>
        <div class="text-xs text-gray-500 dark:text-gray-400 truncate">${connectionType}${additionalInfo}</div>
      </div>
    </div>
  `;
  
  option.addEventListener('click', () => selectPrinter(value, option));
  
  return option;
}

/**
 * Handle printer selection
 */
function selectPrinter(value, element) {
  // Remove selection from all options
  document.querySelectorAll('.printer-option').forEach(option => {
    option.className = option.className.replace(/border-orange-400|bg-orange-50|text-orange-700|dark:bg-orange-900\/20|dark:text-orange-300|dark:border-orange-600/g, '')
                                   .replace(/border-gray-200|bg-gray-50|text-gray-700|dark:border-gray-600|dark:bg-gray-700|dark:text-gray-300/g, '')
                        + ' border-gray-200 dark:border-gray-600 bg-gray-50 dark:bg-gray-700 text-gray-700 dark:text-gray-300 hover:border-gray-300 dark:hover:border-gray-500';
  });
  
  // Add selection to clicked option
  element.className = element.className.replace(/border-gray-200|bg-gray-50|text-gray-700|hover:border-gray-300|dark:border-gray-600|dark:bg-gray-700|dark:text-gray-300|dark:hover:border-gray-500/g, '')
                    + ' border-orange-400 bg-orange-50 dark:bg-orange-900/20 text-orange-700 dark:text-orange-300 dark:border-orange-600';
  
  // Update hidden input
  const hiddenInput = document.getElementById('printer-target');
  if (hiddenInput) {
    hiddenInput.value = value;
  }
}

/**
 * Initialize UI components that depend on config being loaded
 */
function initializeConfigDependentUI() {
  initializePrinterSelection();
}

/**
 * Generic character counter function
 * @param {string} textareaId - ID of the textarea element
 * @param {string} counterId - ID of the counter element  
 * @param {number} defaultMaxLength - Default max length if not set on textarea
 */
function updateCharacterCount(textareaId, counterId, defaultMaxLength = 1000) {
  const textarea = document.getElementById(textareaId);
  const counter = document.getElementById(counterId);
  
  if (textarea && counter) {
    const length = textarea.value.length;
    const maxLength = defaultMaxLength; // Always use the provided limit, not HTML maxlength
    counter.textContent = `${length}/${maxLength} characters`;
    
    // Get the parent div that contains the styling
    const parentDiv = counter.parentElement;
    if (parentDiv) {
      // Remove existing color classes from parent
      parentDiv.classList.remove('text-gray-500', 'text-yellow-700', 'text-red-600', 'dark:text-gray-400', 'dark:text-yellow-300', 'dark:text-red-400');
      
      // Update styling based on character count
      if (length > maxLength) {
        parentDiv.classList.add('text-red-600', 'dark:text-red-400');
      } else if (length >= maxLength * 0.8) {
        parentDiv.classList.add('text-yellow-700', 'dark:text-yellow-300');
      } else {
        parentDiv.classList.add('text-gray-500', 'dark:text-gray-400');
      }
    }
  }
}

/**
 * Navigate to settings page
 */
function goToSettings() {
  window.location.href = '/settings.html';
}
