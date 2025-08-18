/**
 * @file index.js
 * @brief Index page specific functionality
 */

/**
 * Initialize index page when DOM is loaded
 */
document.addEventListener('DOMContentLoaded', async function() {
  // Initialize index-specific UI
  initializeIndexUI(); 
  
  // Check for settings saved success message
  checkForSettingsSuccess();
  
  try {
    // Load configuration for this page
    console.log('📋 Index: Loading configuration...');
    const config = await loadConfigForPage();
    console.log('📋 Index: Config loaded, initializing printer discovery');
    
    // Store config for use by other functions on this page
    window.indexPageConfig = config;
    
    // Initialize printer discovery
    initializePrinterDiscovery();
    
    // Initialize printer selection UI
    initializePrinterSelection();
  } catch (error) {
    console.error('📋 Index: Failed to load config:', error);
    // Continue with defaults - page should still work
    initializePrinterDiscovery();
    initializePrinterSelection();
  }
  
  // Listen for printer updates from the SSE system
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
  const localPrinterName = window.indexPageConfig?.printer?.name || 'Unknown';
  
  // Add local-direct printer option with simple name
  const localDirectOption = createPrinterOption('local-direct', '🏠', 'Local direct', true, null);
  container.appendChild(localDirectOption);
  
  // Add remote printers from PRINTERS config (this will now include discovered printers)
  if (typeof PRINTERS !== 'undefined') {
    PRINTERS.forEach(printer => {
      // Construct the MQTT topic from printer name
      const topic = `scribe/${printer.name}/print`;
      const option = createPrinterOption(topic, '📡', printer.name, false, printer);
      container.appendChild(option);
    });
  }
}

/**
 * Create a printer option element
 */
function createPrinterOption(value, icon, name, isSelected = false, printerData = null) {
  const option = document.createElement('div');
  option.className = `printer-option cursor-pointer p-5 rounded-xl border-2 transition-all duration-200 hover:scale-105 hover:shadow-md ${
    isSelected ? 'border-orange-400 bg-orange-50 dark:bg-orange-900/20 text-orange-700 dark:text-orange-300 dark:border-orange-600' : 'border-gray-200 dark:border-gray-600 bg-gray-50 dark:bg-gray-700 text-gray-700 dark:text-gray-300 hover:border-gray-300 dark:hover:border-gray-500'
  }`;
  option.setAttribute('data-value', value);
  
  // Determine clean display text for main view
  let displayText = '';
  let showInfoIcon = false;
  
  if (value === 'local-direct') {
    displayText = 'Direct connection';
  } else {
    // For MQTT printers, name already includes "via MQTT", so just show that
    displayText = 'Direct connection'; // This will be overridden below for MQTT
    showInfoIcon = true;
  }
  
  // Create the main content structure
  const mainContent = document.createElement('div');
  mainContent.className = 'flex items-center justify-between w-full';
  
  // Create left side with icon and name
  const leftSide = document.createElement('div');
  leftSide.className = 'flex items-center space-x-4 flex-1 min-w-0';
  
  leftSide.innerHTML = `
    <span class="text-3xl">${icon}</span>
    <div class="flex-1 min-w-0 flex items-center">
      <div class="font-medium text-base truncate">${name}</div>
    </div>
  `;
  
  mainContent.appendChild(leftSide);
  
  // Add info icon for all printers (both local and MQTT)
  const infoIcon = document.createElement('div');
  infoIcon.className = 'info-icon ml-3 flex-shrink-0 w-14 h-14 flex items-center justify-center rounded-full transition-colors duration-200 cursor-pointer';
  infoIcon.innerHTML = '<span class="text-2xl text-gray-600 dark:text-gray-300">ⓘ</span>';
  infoIcon.title = 'View printer details';
  
  // Add touch-friendly styling for mobile
  infoIcon.style.minWidth = '56px';
  infoIcon.style.minHeight = '56px';
  
  if (value === 'local-direct') {
    // For local printer, we'll need to get local printer info
    // Store a flag to indicate this is local
    infoIcon.setAttribute('data-is-local', 'true');
    infoIcon.setAttribute('data-printer-name', name.replace('Local direct (', '').replace(')', ''));
    
    // Add click handler for local printer info
    infoIcon.addEventListener('click', (event) => {
      event.stopPropagation(); // Prevent printer selection
      showLocalPrinterInfo();
    });
  } else if (printerData) {
    // For MQTT printers
    infoIcon.setAttribute('data-printer-info', JSON.stringify(printerData));
    infoIcon.setAttribute('data-printer-name', printerData.name || 'Unknown');
    infoIcon.setAttribute('data-printer-topic', `scribe/${printerData.name}/print`);
    
    // Add click handler for MQTT printer info
    infoIcon.addEventListener('click', (event) => {
      event.stopPropagation(); // Prevent printer selection
      showPrinterInfoOverlay(printerData, printerData.name || 'Unknown');
    });
  }
  
  mainContent.appendChild(infoIcon);
  
  option.appendChild(mainContent);
  
  // Add click handler for printer selection to the entire option
  option.addEventListener('click', (event) => {
    // Don't trigger selection if clicking on the info icon
    if (!event.target.closest('.info-icon')) {
      selectPrinter(value, option);
    }
  });
  
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
 * Format time difference to a readable string
 */
function formatTimeDifference(diffMs) {
  const diffSeconds = Math.floor(diffMs / 1000);
  const diffMinutes = Math.floor(diffSeconds / 60);
  const diffHours = Math.floor(diffMinutes / 60);
  const diffDays = Math.floor(diffHours / 24);
  
  if (diffDays > 0) {
    return `${diffDays} day${diffDays > 1 ? 's' : ''} ago`;
  } else if (diffHours > 0) {
    return `about ${diffHours} hour${diffHours > 1 ? 's' : ''} ago`;
  } else if (diffMinutes >= 2) {
    return `${diffMinutes} mins ago`;
  } else if (diffMinutes === 1) {
    return 'A minute ago';
  } else if (diffSeconds > 30) {
    return '30 seconds ago';
  } else {
    return 'Just now';
  }
}

/**
 * Copy text to clipboard with visual feedback
 */
function copyToClipboard(text, buttonElement) {
  // Modern browsers with clipboard API support
  if (navigator.clipboard && window.isSecureContext) {
    navigator.clipboard.writeText(text).then(() => {
      showCopyFeedback(buttonElement);
    }).catch(() => {
      // Fallback if clipboard API fails
      fallbackCopyToClipboard(text, buttonElement);
    });
  } else {
    // Fallback for older browsers or non-HTTPS contexts
    fallbackCopyToClipboard(text, buttonElement);
  }
}

/**
 * Fallback clipboard copy using textarea selection
 */
function fallbackCopyToClipboard(text, buttonElement) {
  // Create a temporary textarea
  const textArea = document.createElement('textarea');
  textArea.value = text;
  textArea.style.position = 'fixed';
  textArea.style.left = '-999999px';
  textArea.style.top = '-999999px';
  document.body.appendChild(textArea);
  
  try {
    textArea.focus();
    textArea.select();
    const successful = document.execCommand('copy');
    if (successful) {
      showCopyFeedback(buttonElement);
    } else {
      console.error('Failed to copy text');
    }
  } catch (err) {
    console.error('Failed to copy:', err);
  } finally {
    document.body.removeChild(textArea);
  }
}

/**
 * Show visual feedback for successful copy
 */
function showCopyFeedback(buttonElement) {
  if (!buttonElement) return;
  
  const originalContent = buttonElement.innerHTML;
  buttonElement.innerHTML = '✅';
  buttonElement.disabled = true;
  
  setTimeout(() => {
    buttonElement.innerHTML = originalContent;
    buttonElement.disabled = false;
  }, 1500);
}

/**
 * Show local printer information overlay
 */
function showLocalPrinterInfo() {
  // Create local printer data object with current information from loaded config
  const deviceConfig = window.indexPageConfig?.device || {};
  const localPrinterData = {
    name: deviceConfig.printer_name || deviceConfig.owner || 'Local Printer',
    ip_address: deviceConfig.ip_address || window.location.hostname,
    mdns: deviceConfig.mdns || window.location.hostname,
    status: 'online',
    firmware_version: deviceConfig.firmware_version || 'Unknown',
    timezone: deviceConfig.timezone || 'Unknown',
    last_power_on: deviceConfig.boot_time || 'Unknown'
  };
  
  showPrinterInfoOverlay(localPrinterData, localPrinterData.name, 'local');
}

/**
 * Show printer information overlay with mobile-responsive design
 */
function showPrinterInfoOverlay(printerData, printerName, printerType = 'mqtt') {
  // Remove any existing overlay
  const existingOverlay = document.getElementById('printer-info-overlay');
  if (existingOverlay) {
    existingOverlay.remove();
  }
  
  // Create overlay container
  const overlay = document.createElement('div');
  overlay.id = 'printer-info-overlay';
  overlay.className = 'fixed inset-0 z-50 flex items-end sm:items-center sm:justify-center p-4';
  
  // Create backdrop with semi-transparent smoke effect
  const backdrop = document.createElement('div');
  backdrop.className = 'absolute inset-0 bg-black/60 backdrop-blur-sm';
  backdrop.addEventListener('click', () => closePrinterInfoOverlay());
  
  // Create modal content
  const modal = document.createElement('div');
  modal.className = 'relative bg-white dark:bg-gray-800 rounded-t-2xl sm:rounded-2xl w-full max-w-md max-h-[80vh] overflow-y-auto shadow-2xl transform transition-all duration-300 ease-out';
  
  // Format the data for display
  const topic = printerType === 'mqtt' ? `scribe/${printerName}/print` : null;
  const ipAddress = printerData.ip_address || 'Not available';
  const mdns = printerData.mdns || `${printerName.toLowerCase()}.local`;
  const firmwareVersion = printerData.firmware_version || 'Not available';
  
  // Choose appropriate icon and title based on printer type
  const printerIcon = printerType === 'local' ? '🏠' : '📡';
  const printerTypeText = printerType === 'local' ? 'Local Printer' : 'Remote Printer';
  
  // Format last power on time with both relative and absolute
  let lastPowerOnRelative = 'Not available';
  let lastPowerOnAbsolute = '';
  if (printerData.last_power_on) {
    // Handle different timestamp formats - could be ISO string, Unix timestamp, or epoch milliseconds
    let powerOnTime;
    if (typeof printerData.last_power_on === 'string') {
      // ISO string - should be proper UTC time with Z suffix
      powerOnTime = new Date(printerData.last_power_on);
    } else if (typeof printerData.last_power_on === 'number') {
      // Unix timestamp - convert to milliseconds if needed
      const timestamp = printerData.last_power_on < 10000000000 ? 
        printerData.last_power_on * 1000 : printerData.last_power_on;
      powerOnTime = new Date(timestamp);
    } else {
      powerOnTime = new Date(printerData.last_power_on);
    }
    
    const now = new Date();
    const diffMs = now.getTime() - powerOnTime.getTime();
    
    lastPowerOnRelative = formatTimeDifference(diffMs);
    // Format in local timezone with locale-specific formatting
    lastPowerOnAbsolute = powerOnTime.toLocaleString(undefined, {
      year: 'numeric',
      month: 'short',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
      hour12: false
      // Browser will automatically use local timezone
    });
  }
  
  const timezone = printerData.timezone || 'Same as local';
  
  modal.innerHTML = `
    <div class="p-6">
      <!-- Header with close button -->
      <div class="flex items-center justify-between mb-6">
        <h3 class="text-lg font-semibold text-gray-900 dark:text-gray-100 flex items-center">
          <span class="text-2xl mr-2">${printerIcon}</span>
          ${printerName} Details
        </h3>
        <button onclick="closePrinterInfoOverlay()" class="w-8 h-8 flex items-center justify-center rounded-full bg-gray-100 dark:bg-gray-700 hover:bg-gray-200 dark:hover:bg-gray-600 transition-colors duration-200">
          <span class="text-gray-600 dark:text-gray-400 text-xl">×</span>
        </button>
      </div>
      
      <!-- Information sections -->
      <div class="space-y-4">
        ${topic ? `
        <div>
          <div class="text-sm font-medium text-gray-700 dark:text-gray-300 mb-1 flex items-center justify-between">
            <span>MQTT Topic</span>
            <button onclick="copyToClipboard('${topic}', this)" class="p-1 hover:bg-gray-100 dark:hover:bg-gray-700 rounded transition-colors duration-200" title="Copy to clipboard">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor" class="text-gray-500 dark:text-gray-400">
                <path d="M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z" />
              </svg>
            </button>
          </div>
          <div class="text-sm bg-gray-50 dark:bg-gray-700 p-3 rounded-lg font-mono text-gray-800 dark:text-gray-200 break-all">
            ${topic}
          </div>
        </div>
        ` : ''}
        
        <div>
          <div class="text-sm text-gray-800 dark:text-gray-200">
            <span class="font-medium text-gray-700 dark:text-gray-300">Local IP Address:</span> ${ipAddress}
          </div>
        </div>
        
        <div>
          <div class="text-sm text-gray-800 dark:text-gray-200">
            <span class="font-medium text-gray-700 dark:text-gray-300">Local mDNS:</span> ${mdns}
          </div>
        </div>
        
        <div>
          <div class="text-sm text-gray-800 dark:text-gray-200">
            <span class="font-medium text-gray-700 dark:text-gray-300">Firmware Version:</span> ${firmwareVersion}
          </div>
        </div>
        
        <div>
          <div class="text-sm text-gray-800 dark:text-gray-200">
            <span class="font-medium text-gray-700 dark:text-gray-300">Last Power On:</span> ${lastPowerOnRelative}${lastPowerOnAbsolute ? ` (${lastPowerOnAbsolute})` : ''}
          </div>
        </div>
        
        <div>
          <div class="text-sm text-gray-800 dark:text-gray-200">
            <span class="font-medium text-gray-700 dark:text-gray-300">Timezone:</span> ${timezone}
          </div>
        </div>
      </div>
    </div>
  `;
  
  overlay.appendChild(backdrop);
  overlay.appendChild(modal);
  document.body.appendChild(overlay);
  
  // Animate in
  requestAnimationFrame(() => {
    overlay.style.opacity = '1';
    modal.style.transform = 'translateY(0)';
  });
  
  // Set initial styles for animation
  overlay.style.opacity = '0';
  modal.style.transform = 'translateY(100%)';
  
  // Handle escape key
  document.addEventListener('keydown', handleOverlayEscape);
}

/**
 * Close printer information overlay
 */
function closePrinterInfoOverlay() {
  const overlay = document.getElementById('printer-info-overlay');
  if (overlay) {
    const modal = overlay.querySelector('.relative');
    
    // Animate out
    overlay.style.opacity = '0';
    if (modal) {
      modal.style.transform = 'translateY(100%)';
    }
    
    setTimeout(() => {
      overlay.remove();
      document.removeEventListener('keydown', handleOverlayEscape);
    }, 300);
  }
}

/**
 * Handle escape key for closing overlay
 */
function handleOverlayEscape(event) {
  if (event.key === 'Escape') {
    closePrinterInfoOverlay();
  }
}

/**
 * Navigate to settings page
 */
function goToSettings() {
  window.location.href = '/settings.html';
}

/**
 * Check for settings success parameter and show toast
 */
function checkForSettingsSuccess() {
  const urlParams = new URLSearchParams(window.location.search);
  if (urlParams.get('settings_saved') === 'true') {
    // Show success toast
    showSuccessToast('💾 Settings saved');
    
    // Clean up the URL by removing the parameter
    const cleanUrl = window.location.pathname;
    window.history.replaceState({}, document.title, cleanUrl);
  }
}

/**
 * Show success toast message
 */
function showSuccessToast(message) {
  // Create toast container if it doesn't exist
  let container = document.getElementById('toast-container');
  if (!container) {
    container = document.createElement('div');
    container.id = 'toast-container';
    container.className = 'fixed top-4 right-4 z-50 space-y-2';
    document.body.appendChild(container);
  }
  
  // Create toast element
  const toast = document.createElement('div');
  toast.className = 'bg-green-100 text-green-800 border border-green-200 p-4 rounded-lg shadow-lg transition-all duration-300 transform translate-x-0 opacity-100 max-w-sm';
  
  toast.innerHTML = `
    <div class="flex items-center space-x-2">
      ${message.match(/^[\u{1F000}-\u{1F9FF}]|^[\u{2600}-\u{26FF}]/u) ? '' : '<span class="text-xl">✅</span>'}
      <span class="flex-1">${message}</span>
      <button onclick="this.parentElement.parentElement.remove()" class="ml-2 text-lg font-bold opacity-50 hover:opacity-100">×</button>
    </div>
  `;
  
  container.appendChild(toast);
  
  // Auto-remove after 4 seconds
  setTimeout(() => {
    if (toast.parentElement) {
      toast.classList.add('translate-x-full', 'opacity-0');
      setTimeout(() => {
        if (toast.parentElement) {
          toast.remove();
        }
      }, 300);
    }
  }, 4000);
}
