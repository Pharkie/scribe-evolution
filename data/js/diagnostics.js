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
 * Load and display diagnostics data for the diagnostics page
 */
async function loadDiagnostics() {
  try {
    // Fetch diagnostics data
    const response = await fetch('/status');
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    const data = await response.json();
    
    await displayDiagnostics(data);
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
    // Get current uptime from data.uptime if available
    const currentUptime = window.lastDiagnosticsData?.uptime || 0;
    
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
async function displayDiagnostics(data) {
  // Store data globally for timestamp formatting
  window.lastDiagnosticsData = data;
  
  // Hide loading indicator
  const loadingIndicator = document.getElementById('loading-indicator');
  if (loadingIndicator) {
    loadingIndicator.classList.add('hidden');
  }

  // Calculate derived values
  const uptimeSeconds = Math.floor(data.uptime / 1000);
  const uptimeMinutes = Math.floor(uptimeSeconds / 60);
  const uptimeHours = Math.floor(uptimeMinutes / 60);
  
  const memoryUsedPercent = Math.round(((data.total_heap - data.free_heap) / data.total_heap) * 100);
  const flashUsedPercent = Math.round((data.flash_used / data.flash_total) * 100);
  const sketchUsedPercent = Math.round((data.sketch_size / (data.sketch_size + data.free_sketch_space)) * 100);
  
  // Show and populate device configuration section
  const deviceConfigSection = document.getElementById('device-config-section');
  if (deviceConfigSection) {
    const configData = data.configuration || {};
    populateDataFields(deviceConfigSection, {
      'device-owner': data.device_owner || 'Unknown',
      'timezone': data.timezone || 'Not configured',
      'mdns-hostname': data.mdns_hostname || 'Unknown',
      'max-message-chars': configData.max_message_chars || 'Unknown'
    });
    deviceConfigSection.classList.remove('hidden');
  }

  // Show and populate config file section - fetch from /config endpoint
  const configFileSection = document.getElementById('config-file-section');
  if (configFileSection) {
    const configFileContents = document.getElementById('config-file-contents');
    const configFileSize = document.querySelector('[data-field="config-file-size"]');
    
    if (configFileContents) {
      try {
        // Fetch the full config from /config endpoint
        const configResponse = await fetch('/config');
        if (configResponse.ok) {
          const configData = await configResponse.json();
          
          // Pretty print the configuration JSON with syntax highlighting
          const configJson = JSON.stringify(configData, null, 2);
          configFileContents.innerHTML = `<code class="language-json">${escapeHtml(configJson)}</code>`;
          
          // Calculate and display file size
          const sizeInBytes = new Blob([configJson]).size;
          const sizeDisplay = sizeInBytes < 1024 ? `${sizeInBytes} B` : `${(sizeInBytes / 1024).toFixed(1)} KB`;
          if (configFileSize) {
            configFileSize.textContent = sizeDisplay;
          }
        } else {
          configFileContents.innerHTML = `<code class="text-red-500">Error fetching config: HTTP ${configResponse.status}</code>`;
          if (configFileSize) {
            configFileSize.textContent = '0 B';
          }
        }
      } catch (error) {
        configFileContents.innerHTML = `<code class="text-red-500">Error: ${escapeHtml(error.message)}</code>`;
        if (configFileSize) {
          configFileSize.textContent = '0 B';
        }
      }
    }
    configFileSection.classList.remove('hidden');
  }
    
  // Show and populate network section
  const networkSection = document.getElementById('network-section');
  if (networkSection) {
    populateDataFields(networkSection, {
      'wifi-status': data.wifi_connected ? 'Connected' : 'Disconnected',
      'wifi-ssid': data.wifi_ssid || 'Not connected',
      'ip-address': data.ip_address || 'Not assigned',
      'signal-strength': data.rssi ? `${data.rssi} dBm` : 'Unknown',
      'mac-address': data.mac_address || 'Unknown',
      'gateway': data.gateway || 'Unknown',
      'dns': data.dns || 'Unknown'
    });
    networkSection.classList.remove('hidden');
  }
  
  // Show and populate MQTT section
  const mqttSection = document.getElementById('mqtt-section');
  if (mqttSection) {
    populateDataFields(mqttSection, {
      'mqtt-status': data.mqtt_connected ? 'Connected' : 'Disconnected',
      'mqtt-server': data.mqtt_server || 'Not configured',
      'mqtt-port': data.mqtt_port || 'Unknown',
      'mqtt-topic': data.local_topic || 'Not configured'
    });
    mqttSection.classList.remove('hidden');
  }
  
  // Show and populate Unbidden Ink section
  const unbiddenSection = document.getElementById('unbidden-ink-section');
  if (unbiddenSection) {
    const unbiddenInkData = data.unbidden_ink || {};
    const configData = data.configuration || {};
    const unbiddenInkEnabled = unbiddenInkData.enabled;
    
    populateDataFields(unbiddenSection, {
      'unbidden-ink-status': unbiddenInkEnabled ? 'Enabled' : 'Disabled',
      'working-hours': unbiddenInkData.start_hour && unbiddenInkData.end_hour ? 
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
  
  // Show and populate microcontroller section
  const microcontrollerSection = document.getElementById('microcontroller-section');
  if (microcontrollerSection) {
    populateDataFields(microcontrollerSection, {
      'chip-model': data.chip_model || 'Unknown',
      'cpu-frequency': data.cpu_freq ? `${data.cpu_freq} MHz` : 'Unknown',
      'temperature': data.temperature ? `${data.temperature}°C` : 'Unknown',
      'uptime': uptimeHours > 0 ? `${uptimeHours}h ${uptimeMinutes % 60}m` : `${uptimeMinutes}m`,
      'reset-reason': data.reset_reason || 'Unknown',
      'heap-usage': `${Math.round((data.total_heap - data.free_heap) / 1024)} KB / ${Math.round(data.total_heap / 1024)} KB (${memoryUsedPercent}%)`,
      'flash-usage': `${Math.round(data.flash_used / 1024)} KB / ${Math.round(data.flash_total / 1024)} KB (${flashUsedPercent}%)`,
      'sketch-usage': `${Math.round(data.sketch_size / 1024)} KB / ${Math.round((data.sketch_size + data.free_sketch_space) / 1024)} KB (${sketchUsedPercent}%)`,
      'heap-bar': `${memoryUsedPercent}%`,
      'flash-bar': `${flashUsedPercent}%`,
      'sketch-bar': `${sketchUsedPercent}%`
    });
    microcontrollerSection.classList.remove('hidden');
  }
  
  // Show and populate hardware buttons section
  const hardwareButtonsSection = document.getElementById('hardware-buttons-section');
  if (hardwareButtonsSection && data.hardware_buttons) {
    const buttonsContent = document.getElementById('hardware-buttons-content');
    if (buttonsContent) {
      buttonsContent.innerHTML = ''; // Clear existing content
      
      // Add hardware button configuration attributes
      const configAttributes = [
        { label: 'Number of Buttons', value: data.hardware_buttons.num_buttons || 'Unknown' },
        { label: 'Debounce Time', value: data.hardware_buttons.debounce_ms ? `${data.hardware_buttons.debounce_ms} ms` : 'Unknown' },
        { label: 'Long Press Time', value: data.hardware_buttons.long_press_ms ? `${data.hardware_buttons.long_press_ms} ms` : 'Unknown' },
        { label: 'Active Low', value: data.hardware_buttons.active_low !== undefined ? (data.hardware_buttons.active_low ? 'Yes' : 'No') : 'Unknown' },
        { label: 'Min Interval', value: data.hardware_buttons.min_interval_ms ? `${data.hardware_buttons.min_interval_ms} ms` : 'Unknown' },
        { label: 'Max Per Minute', value: data.hardware_buttons.max_per_minute || 'Unknown' }
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
      if (data.hardware_buttons.buttons && data.hardware_buttons.buttons.length > 0) {
        const separator = document.createElement('div');
        separator.className = 'border-t border-gray-200 dark:border-gray-600 my-3 pt-3';
        separator.innerHTML = '<div class="text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Button Mappings:</div>';
        buttonsContent.appendChild(separator);
      }
      
      // Add individual button configurations
      if (data.hardware_buttons.buttons) {
        data.hardware_buttons.buttons.forEach(button => {
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
  
  // Show and populate logging section
  const loggingSection = document.getElementById('logging-section');
  if (loggingSection) {
    populateDataFields(loggingSection, {
      'log-level': data.logging?.level_name || 'Unknown',
      'serial-logging': data.logging?.serial_enabled ? 'Enabled' : 'Disabled',
      'file-logging': data.logging?.file_enabled ? 'Enabled' : 'Disabled',
      'mqtt-logging': data.logging?.mqtt_enabled ? 'Enabled' : 'Disabled',
      'betterstack-logging': data.logging?.betterstack_enabled ? 'Enabled' : 'Disabled'
    });
    loggingSection.classList.remove('hidden');
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
  const section = document.getElementById(sectionId + '-section');
  if (!section) return;
  
  // Create a plain text version for copying
  let textContent = '🪄 Unbidden Ink\n\n';
  
  const items = section.querySelectorAll('.flex.justify-between');
  items.forEach(item => {
    const label = item.querySelector('.text-gray-600');
    const value = item.querySelector('.font-medium');
    if (label && value) {
      textContent += `${label.textContent.trim()}: ${value.textContent.trim()}\n`;
    }
  });
  
  // Add file contents if present
  const fileContents = document.getElementById('unbidden-ink-file-contents');
  if (fileContents && fileContents.textContent.trim()) {
    textContent += '\nSettings File Contents:\n';
    textContent += fileContents.textContent;
  }
  
  copyToClipboard(textContent, buttonElement);
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
 */
function copyGenericSection(sectionName, buttonElement) {
  // Find the parent section
  const section = buttonElement.closest('.rounded-lg');
  if (!section) return;
  
  let textContent = `${sectionName}\n\n`;
  
  // Get all data rows
  const dataRows = section.querySelectorAll('.flex.justify-between');
  dataRows.forEach(row => {
    const label = row.querySelector('.text-gray-600');
    const value = row.querySelector('.font-medium');
    if (label && value) {
      // Clean up the text content
      const labelText = label.textContent.trim().replace(':', '');
      const valueText = value.textContent.trim();
      textContent += `${labelText}: ${valueText}\n`;
    }
  });
  
  copyToClipboard(textContent, buttonElement);
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
  buttonElement.innerHTML = '✅';
  buttonElement.disabled = true;
  
  setTimeout(() => {
    buttonElement.innerHTML = originalContent;
    buttonElement.disabled = false;
  }, 1500);
}
