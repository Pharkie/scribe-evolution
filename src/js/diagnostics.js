/**
 * @file diagnostics.js
 * @brief System diagnostics and status display functionality
 */

/**
 * Utility function to escape HTML for safe display
 */
function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

/**
 * Redact secrets from configuration data for safe display
 */
function redactSecrets(configData) {
  // Create a deep copy to avoid modifying the original
  const redacted = JSON.parse(JSON.stringify(configData));
  
  // List of keys that should be redacted (case-insensitive partial matches)
  const secretKeys = [
    'password', 'pass', 'secret', 'token', 'key', 'apikey', 'api_key',
    'auth', 'credential', 'cert', 'private', 'bearer', 'oauth'
  ];
  
  function redactObject(obj, path = '') {
    if (typeof obj !== 'object' || obj === null) {
      return obj;
    }
    
    for (const [key, value] of Object.entries(obj)) {
      const lowerKey = key.toLowerCase();
      const shouldRedact = secretKeys.some(secretKey => lowerKey.includes(secretKey));
      
      if (shouldRedact && typeof value === 'string' && value.length > 0) {
        // Redact the value, showing only first 2 and last 2 characters for longer strings
        if (value.length > 8) {
          obj[key] = value.substring(0, 2) + '●●●●●●●●' + value.substring(value.length - 2);
        } else {
          obj[key] = '●●●●●●●●';
        }
      } else if (typeof value === 'object' && value !== null) {
        redactObject(value, path + '.' + key);
      }
    }
  }
  
  redactObject(redacted);
  return redacted;
}

/**
 * Load and display diagnostics data for the diagnostics page
 */
async function loadDiagnostics() {
  try {
    // Wait for global config to be loaded using event listener (more reliable)
    if (!window.GLOBAL_CONFIG || Object.keys(window.GLOBAL_CONFIG).length === 0) {
      console.log('Waiting for global config to load...');
      
      // Use event-based waiting with fallback timeout
      await new Promise((resolve, reject) => {
        const timeout = setTimeout(() => {
          reject(new Error('Global config not loaded within timeout - please refresh the page'));
        }, 10000); // 10 second timeout
        
        const handleConfigLoaded = (event) => {
          clearTimeout(timeout);
          window.removeEventListener('configLoaded', handleConfigLoaded);
          resolve(event.detail);
        };
        
        // If config is already loaded, resolve immediately
        if (window.GLOBAL_CONFIG && Object.keys(window.GLOBAL_CONFIG).length > 0) {
          clearTimeout(timeout);
          resolve(window.GLOBAL_CONFIG);
          return;
        }
        
        // Otherwise wait for the event
        window.addEventListener('configLoaded', handleConfigLoaded);
      });
    }
    
    // Load diagnostics data from the server
    const statusResponse = await fetch('/api/diagnostics');
    
    if (!statusResponse.ok) {
      throw new Error(`Status endpoint error: HTTP ${statusResponse.status}: ${statusResponse.statusText}`);
    }
    
    const statusData = await statusResponse.json();
    
    // Use the already-loaded global config
    await displayDiagnostics(statusData, window.GLOBAL_CONFIG);
  } catch (error) {
    console.error('Failed to load diagnostics:', error);
    displayDiagnosticsError(error.message);
  }
}

/**
 * Format a timestamp to a readable date/time string
 * Handles millis() values from ESP32 (milliseconds since device boot)
 */
function formatTimestamp(timestamp) {
  if (!timestamp || timestamp == 0) return 'Not scheduled';
  
  // For ESP32 millis() values, show relative time
  if (timestamp < 946684800000) { // Less than year 2000 in milliseconds - this is millis() since boot
    // Get current uptime from data.system.uptime_ms if available
    const currentUptime = window.lastDiagnosticsData?.system?.uptime_ms || 0;
    
    if (currentUptime > 0) {
      // Calculate relative time from current uptime
      const timeDiffMs = timestamp - currentUptime;
      
      if (timeDiffMs <= 0) {
        return 'Overdue';
      }
      
      const minutes = Math.floor(timeDiffMs / (1000 * 60));
      const hours = Math.floor(minutes / 60);
      const days = Math.floor(hours / 24);
      
      if (days > 0) {
        return `In ${days} day(s), ${hours % 24} hour(s)`;
      } else if (hours > 0) {
        return `In ${hours} hour(s), ${minutes % 60} minute(s)`;
      } else if (minutes > 0) {
        return `In ${minutes} minute(s)`;
      } else {
        return `In ${Math.floor(timeDiffMs / 1000)} second(s)`;
      }
    } else {
      // Fallback to absolute time calculation (less accurate)
      const minutes = Math.floor(timestamp / (1000 * 60));
      const hours = Math.floor(minutes / 60);
      const days = Math.floor(hours / 24);
      
      if (days > 0) {
        return `In ${days} day(s)`;
      } else if (hours > 0) {
        return `In ${hours} hour(s)`;
      } else if (minutes > 0) {
        return `In ${minutes} minute(s)`;
      } else {
        return 'Very soon';
      }
    }
  }
  
  // For actual timestamps (epoch time)
  const date = new Date(timestamp * 1000);
  return date.toLocaleString();
}

/**
 * Helper function to populate data fields in an element
 */
function populateDataFields(element, data) {
  Object.keys(data).forEach(key => {
    const field = element.querySelector(`[data-field="${key}"]`);
    if (field) {
      if (key.includes('-bar')) {
        // Handle progress bars
        field.style.width = data[key];
      } else {
        // Handle text content
        field.textContent = data[key];
      }
    }
  });
}

/**
 * Display diagnostics data in the page elements
 */
async function displayDiagnostics(data, configData) {
  // Store data globally for timestamp formatting
  window.lastDiagnosticsData = data;
  
  // Hide loading indicator
  const loadingIndicator = document.getElementById('loading-indicator');
  if (loadingIndicator) {
    loadingIndicator.classList.add('hidden');
  }

  // Calculate derived values
  const uptimeSeconds = Math.floor(data.system.uptime_ms / 1000);
  const uptimeMinutes = Math.floor(uptimeSeconds / 60);
  const uptimeHours = Math.floor(uptimeMinutes / 60);
  
  const memoryUsedPercent = Math.round((data.system.memory.used_heap / data.system.memory.total_heap) * 100);
  
  // 1. Show and populate device configuration section
  const deviceConfigSection = document.getElementById('device-config-section');
  if (deviceConfigSection) {
    populateDataFields(deviceConfigSection, {
      'device-owner': data.device?.owner || configData.device?.owner || 'Unknown',
      'timezone': data.device?.timezone || configData.device?.timezone || 'Not configured',
      'mdns-hostname': data.device?.hostname || 'Unknown',
      'max-message-chars': data.configuration?.max_message_chars || configData.validation?.maxCharacters || 'Unknown'
    });
    deviceConfigSection.classList.remove('hidden');
  }
    
  // 2. Show and populate network section
  const networkSection = document.getElementById('network-section');
  if (networkSection) {
    populateDataFields(networkSection, {
      'wifi-status': data.network?.wifi?.connected ? 'Connected' : 'Disconnected',
      'wifi-ssid': data.network?.wifi?.ssid || 'Not connected',
      'ip-address': data.network?.wifi?.ip_address || 'Not assigned',
      'signal-strength': data.network?.wifi?.signal_strength_dbm ? `${data.network.wifi.signal_strength_dbm} dBm` : 'Unknown',
      'mac-address': data.network?.wifi?.mac_address || 'Unknown',
      'gateway': data.network?.wifi?.gateway || 'Unknown',
      'dns': data.network?.wifi?.dns || 'Unknown',
      'wifi-connect-timeout': data.network?.wifi?.connect_timeout_ms ? `${Math.round(data.network.wifi.connect_timeout_ms / 1000)} seconds` : '15 seconds'
    });
    networkSection.classList.remove('hidden');
  }
  
  // 3. Show and populate MQTT section
  const mqttSection = document.getElementById('mqtt-section');
  if (mqttSection) {
    populateDataFields(mqttSection, {
      'mqtt-status': data.network?.mqtt?.connected ? 'Connected' : 'Disconnected',
      'mqtt-server': data.network?.mqtt?.server || 'Not configured',
      'mqtt-port': data.network?.mqtt?.port || 'Unknown',
      'mqtt-topic': data.network?.mqtt?.topic || 'Not configured'
    });
    mqttSection.classList.remove('hidden');
  }
  
  // 4. Show and populate microcontroller section
  const microcontrollerSection = document.getElementById('microcontroller-section');
  if (microcontrollerSection) {
    // Calculate storage percentages and formats
    const memoryUsedPercent = Math.round((data.system.memory.used_heap / data.system.memory.total_heap) * 100);
    
    // Enhanced flash calculations with app partition info
    const appUsedPercent = Math.round((data.system.flash.app_partition.used / data.system.flash.app_partition.total) * 100);
    const fsUsedPercent = Math.round((data.system.flash.filesystem.used / data.system.flash.filesystem.total) * 100);
    
    // Flash allocation percentages (what portion of total flash each partition takes)
    const appPartitionPercent = data.system.flash.app_partition.percent_of_total_flash || 0;
    const fsPartitionPercent = data.system.flash.filesystem.percent_of_total_flash || 0;
    
    // Format storage displays
    const formatBytes = (bytes) => {
      if (bytes < 1024) return `${bytes} B`;
      return `${Math.round(bytes / 1024)} KB`;
    };
    
    // Memory display
    const memoryUsageText = `${formatBytes(data.system.memory.used_heap)} / ${formatBytes(data.system.memory.total_heap)} (${memoryUsedPercent}%)`;
    
    // App partition display with both usage and allocation info
    const appUsageText = `${formatBytes(data.system.flash.app_partition.used)} / ${formatBytes(data.system.flash.app_partition.total)} (${appUsedPercent}%)`;
    
    // File system display
    const fsUsageText = `${formatBytes(data.system.flash.filesystem.used)} / ${formatBytes(data.system.flash.filesystem.total)} (${fsUsedPercent}%)`;
    
    populateDataFields(microcontrollerSection, {
      'chip-model': data.hardware?.chip_model || 'Unknown',
      'cpu-frequency': data.hardware?.cpu_frequency_mhz ? `${data.hardware.cpu_frequency_mhz} MHz` : 'Unknown',
      'temperature': data.hardware?.temperature ? `${data.hardware.temperature}°C` : 'Unknown',
      'uptime': uptimeHours > 0 ? `${uptimeHours}h ${uptimeMinutes % 60}m` : `${uptimeMinutes}m`,
      'reset-reason': data.hardware?.reset_reason || 'Unknown',
      'heap-usage': memoryUsageText,
      'app-partition-usage': appUsageText,
      'filesystem-usage': fsUsageText,
      'heap-bar': `${memoryUsedPercent}%`,
      'app-partition-bar': `${appUsedPercent}%`,
      'filesystem-bar': `${fsUsedPercent}%`
    });
    
    microcontrollerSection.classList.remove('hidden');
  }
  
  // 5. Show and populate Unbidden Ink section
  const unbiddenSection = document.getElementById('unbidden-ink-section');
  if (unbiddenSection) {
    const unbiddenInkData = data.features?.unbidden_ink || {};
    const configData = data.configuration || {};
    const unbiddenInkEnabled = unbiddenInkData.enabled;
    
    populateDataFields(unbiddenSection, {
      'unbidden-ink-status': unbiddenInkEnabled ? 'Enabled' : 'Disabled',
      'working-hours': unbiddenInkData.start_hour !== undefined && unbiddenInkData.end_hour !== undefined ? 
        `${unbiddenInkData.start_hour}:00 - ${unbiddenInkData.end_hour}:00` : 'Not configured',
      'frequency': unbiddenInkData.frequency_minutes ? 
        `${unbiddenInkData.frequency_minutes} minutes` : 'Not configured',
      'next-scheduled': formatTimestamp(unbiddenInkData.next_message_time),
      'ai-prompt-char-limit': configData.max_prompt_chars ? 
        `${configData.max_prompt_chars} characters` : 'Unknown',
      'settings-file-size': configData.unbidden_ink_settings_file_size ? 
        `${configData.unbidden_ink_settings_file_size} bytes` : 'Unknown'
    });
    
    // Populate file contents
    const fileContentsElement = document.getElementById('unbidden-ink-file-contents');
    if (fileContentsElement && configData.unbidden_ink_settings_file_contents) {
      fileContentsElement.textContent = configData.unbidden_ink_settings_file_contents;
    } else if (fileContentsElement) {
      fileContentsElement.textContent = 'No file data available';
    }
    
    unbiddenSection.classList.remove('hidden');
  }
  
  // 6. Show and populate logging section
  const loggingSection = document.getElementById('logging-section');
  if (loggingSection) {
    const loggingData = data.features?.logging || {};
    populateDataFields(loggingSection, {
      'log-level': loggingData.level_name || 'Unknown',
      'serial-logging': loggingData.serial_enabled ? 'Enabled' : 'Disabled',
      'file-logging': loggingData.file_enabled ? 'Enabled' : 'Disabled',
      'mqtt-logging': loggingData.mqtt_enabled ? 'Enabled' : 'Disabled',
      'betterstack-logging': loggingData.betterstack_enabled ? 'Enabled' : 'Disabled'
    });
    loggingSection.classList.remove('hidden');
  }
  
  // 7. Show and populate hardware buttons section
  const hardwareButtonsSection = document.getElementById('hardware-buttons-section');
  if (hardwareButtonsSection && data.features?.hardware_buttons) {
    const buttonsContent = document.getElementById('hardware-buttons-content');
    if (buttonsContent) {
      buttonsContent.innerHTML = ''; // Clear existing content
      
      const buttonsData = data.features.hardware_buttons;
      
      // Add hardware button configuration attributes
      const configAttributes = [
        { label: 'Number of Buttons', value: buttonsData.num_buttons || 'Unknown' },
        { label: 'Debounce Time', value: buttonsData.debounce_ms ? `${buttonsData.debounce_ms} ms` : 'Unknown' },
        { label: 'Long Press Time', value: buttonsData.long_press_ms ? `${buttonsData.long_press_ms} ms` : 'Unknown' },
        { label: 'Active Low', value: buttonsData.active_low !== undefined ? (buttonsData.active_low ? 'Yes' : 'No') : 'Unknown' },
        { label: 'Min Interval', value: buttonsData.min_interval_ms ? `${buttonsData.min_interval_ms} ms` : 'Unknown' },
        { label: 'Max Per Minute', value: buttonsData.max_per_minute || 'Unknown' }
      ];
      
      configAttributes.forEach(attr => {
        const configEntry = document.createElement('div');
        configEntry.className = 'flex justify-between';
        configEntry.innerHTML = `
          <span class="text-gray-600 dark:text-gray-400">${attr.label}:</span>
          <span class="font-medium text-gray-900 dark:text-gray-100">${attr.value}</span>
        `;
        buttonsContent.appendChild(configEntry);
      });
      
      // Add a separator if we have both config and button data
      if (buttonsData.buttons && buttonsData.buttons.length > 0) {
        const separator = document.createElement('div');
        separator.className = 'border-t border-gray-200 dark:border-gray-600 my-3 pt-3';
        separator.innerHTML = '<div class="text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Button Mappings:</div>';
        buttonsContent.appendChild(separator);
      }
      
      // Add individual button configurations
      if (buttonsData.buttons) {
        buttonsData.buttons.forEach(button => {
          const buttonEntry = document.createElement('div');
          buttonEntry.className = 'flex justify-between';
          buttonEntry.innerHTML = `
            <span class="text-gray-600 dark:text-gray-400">Pin ${button.gpio}:</span>
            <span class="font-medium text-gray-900 dark:text-gray-100">Short: ${button.short_endpoint || 'None'}, Long: ${button.long_endpoint || 'None'}</span>
          `;
          buttonsContent.appendChild(buttonEntry);
        });
      }
    }
    hardwareButtonsSection.classList.remove('hidden');
  }

  // 9. Show and populate config file section - use the configData passed in
  const configFileSection = document.getElementById('config-file-section');
  if (configFileSection) {
    const configFileContents = document.getElementById('config-file-contents');
    const configFileSize = document.querySelector('[data-field="config-file-size"]');
    
    if (configFileContents) {
      try {
        // Redact secrets before displaying
        const redactedConfig = redactSecrets(configData);
        
        // Pretty print the configuration JSON with syntax highlighting
        const configJson = JSON.stringify(redactedConfig, null, 2);
        configFileContents.innerHTML = `<code class="language-json">${escapeHtml(configJson)}</code>`;
        
        // Calculate and display file size (of original, not redacted)
        const originalJson = JSON.stringify(configData, null, 2);
        const sizeInBytes = new Blob([originalJson]).size;
        const sizeDisplay = sizeInBytes < 1024 ? `${sizeInBytes} B` : `${(sizeInBytes / 1024).toFixed(1)} KB`;
        if (configFileSize) {
          configFileSize.textContent = sizeDisplay;
        }
      } catch (error) {
        configFileContents.innerHTML = `<code class="text-red-500">Error processing config: ${escapeHtml(error.message)}</code>`;
        if (configFileSize) {
          configFileSize.textContent = '0 B';
        }
      }
    }
    configFileSection.classList.remove('hidden');
  }
}

/**
 * Display an error message when diagnostics fail to load
 */
function displayDiagnosticsError(message) {
  // Hide loading indicator
  const loadingIndicator = document.getElementById('loading-indicator');
  if (loadingIndicator) {
    loadingIndicator.classList.add('hidden');
  }

  // Show error container
  const errorContainer = document.getElementById('error-container');
  const errorMessage = document.getElementById('error-message');
  
  if (errorContainer && errorMessage) {
    errorMessage.textContent = message;
    errorContainer.classList.remove('hidden');
  }
}

/**
 * Copy section content to clipboard
 */
function copySection(sectionId, buttonElement) {
  /**
 * Copy config file contents to clipboard
 */
}

/**
 * Copy file contents to clipboard
 */
function copyFileContents(buttonElement) {
  // Look for the new integrated file contents in Unbidden Ink section
  const fileContents = document.getElementById('unbidden-ink-file-contents') || document.getElementById('file-contents');
  if (!fileContents) return;
  
  copyToClipboard(fileContents.textContent, buttonElement);
}

/**
 * Copy generic section content to clipboard
 * Handles all section types including navigation, data rows, and special content
 */
function copyGenericSection(sectionName, buttonElement) {
  // Find the parent section
  const section = buttonElement.closest('.rounded-lg');
  if (!section) return;

  let textContent = `${sectionName}\n\n`;

  // Handle navigation-style sections with groups and lists
  const groups = section.querySelectorAll('div.space-y-4 > div');
  if (groups.length > 0) {
    groups.forEach(group => {
      const groupTitle = group.querySelector('strong');
      if (groupTitle) {
        textContent += `${groupTitle.textContent.replace(/\s+/g, ' ').trim()}\n`;
      }
      
      const links = group.querySelectorAll('ul li');
      links.forEach(linkItem => {
        const link = linkItem.querySelector('a');
        const desc = linkItem.querySelector('span.text-gray-600, span.text-gray-400');
        
        if (link) {
          // Clean up the link text and format nicely
          const linkText = link.textContent.replace(/\s+/g, ' ').trim();
          const href = link.getAttribute('href');
          const descText = desc ? desc.textContent.replace(/\s+/g, ' ').trim() : '';
          
          textContent += `- ${linkText} (${href}) ${descText}\n`;
        } else {
          // For API endpoints and other list items, clean up the text content
          const itemText = linkItem.textContent.replace(/\s+/g, ' ').trim();
          textContent += `- ${itemText}\n`;
        }
      });
      textContent += '\n'; // Add spacing between groups
    });
  } else {
    // Handle standard data row sections (Device Config, Network, MQTT, etc.)
    // Look for both standard and flexible layout rows
    const dataRows = section.querySelectorAll('.flex.justify-between, .flex.flex-col.sm\\:flex-row.sm\\:justify-between');
    if (dataRows.length > 0) {
      dataRows.forEach(row => {
        const label = row.querySelector('.text-gray-600, .text-gray-400');
        const value = row.querySelector('.font-medium');
        if (label && value) {
          const labelText = label.textContent.replace(/\s+/g, ' ').trim().replace(':', '');
          const valueText = value.textContent.replace(/\s+/g, ' ').trim();
          textContent += `${labelText}: ${valueText}\n`;
        }
      });
    } else {
      // Fallback: extract all meaningful text content, excluding buttons and SVGs
      const textElements = section.querySelectorAll('span, div, p, li');
      const meaningfulTexts = [];
      
      textElements.forEach(el => {
        // Skip buttons, SVGs, and very short text
        if (el.closest('button') || el.closest('svg') || el.textContent.trim().length < 3) return;
        
        const text = el.textContent.replace(/\s+/g, ' ').trim();
        // Avoid duplicates and very generic text
        if (text && !meaningfulTexts.includes(text) && !text.match(/^(-|—|•|\d+)$/)) {
          meaningfulTexts.push(text);
        }
      });
      
      meaningfulTexts.forEach(text => {
        textContent += `${text}\n`;
      });
    }
  }

  copyToClipboard(textContent.trim(), buttonElement);
}

/**
 * Copy config file contents to clipboard
 */
function copyConfigFile(buttonElement) {
  const configFileContents = document.getElementById('config-file-contents');
  if (!configFileContents) return;
  
  const configText = configFileContents.textContent || configFileContents.innerText;
  copyToClipboard(configText, buttonElement);
}

/**
 * Universal clipboard copy function with fallback support
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
      showErrorMessage('Copy failed - please select and copy manually');
    }
  } catch (err) {
    console.error('Failed to copy:', err);
    showErrorMessage('Copy not supported - please select and copy manually');
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
  buttonElement.innerHTML = '🫡';
  buttonElement.disabled = true;
  
  setTimeout(() => {
    buttonElement.innerHTML = originalContent;
    buttonElement.disabled = false;
  }, 1500);
}
