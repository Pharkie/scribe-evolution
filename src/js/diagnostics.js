// System diagnostics and status display functionality

function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

function redactSecrets(configData) {
  const redacted = JSON.parse(JSON.stringify(configData));
  const secretKeys = ['password', 'pass', 'secret', 'token', 'key', 'apikey', 'api_key', 'auth', 'credential', 'cert', 'private', 'bearer', 'oauth'];
  
  function redactObject(obj) {
    if (typeof obj !== 'object' || obj === null) return obj;
    for (const [key, value] of Object.entries(obj)) {
      const lowerKey = key.toLowerCase();
      const shouldRedact = secretKeys.some(secretKey => lowerKey.includes(secretKey));
      if (shouldRedact && typeof value === 'string' && value.length > 0) {
        obj[key] = value.length > 8 ? value.substring(0, 2) + '●●●●●●●●' + value.substring(value.length - 2) : '●●●●●●●●';
      } else if (typeof value === 'object' && value !== null) {
        redactObject(value);
      }
    }
  }
  redactObject(redacted);
  return redacted;
}

let currentSection = 'device-config-section';

function showSection(sectionId) {
  document.querySelectorAll('[id$="-section"]').forEach(section => {
    section.classList.remove('show');
  });
  document.querySelectorAll('.section-nav-btn').forEach(btn => {
    btn.classList.remove('active');
  });
  
  const section = document.getElementById(sectionId);
  if (section) {
    section.classList.add('show');
    currentSection = sectionId;
  }
  
  const activeBtn = event?.target;
  if (activeBtn) {
    activeBtn.classList.add('active');
  }
}

function goBack() {
  window.location.href = '/';
}

function copyGenericSection(sectionName, button) {
  try {
    const section = button.closest('[id$="-section"]');
    if (!section) throw new Error('Section not found');

    const rows = section.querySelectorAll('.diag-row, .space-y-1 > div, .space-y-2 > div');
    let content = `=== ${sectionName} ===\n\n`;

    rows.forEach(row => {
      const label = row.querySelector('.diag-label, .text-gray-600, .dark\\:text-gray-400');
      const value = row.querySelector('.diag-value, .font-medium, [data-field]');
      
      if (label && value) {
        const labelText = label.textContent?.trim().replace(/[:：]\s*$/, '') || '';
        const valueText = value.textContent?.trim() || '';
        if (labelText && valueText) {
          content += `${labelText}: ${valueText}\n`;
        }
      }
    });

    const referenceBoxes = section.querySelectorAll('.diag-reference-box, .bg-blue-100, .bg-green-100, .bg-orange-100');
    referenceBoxes.forEach(box => {
      const items = box.querySelectorAll('div:not(.space-y-1):not(.space-y-2)');
      items.forEach(item => {
        const text = item.textContent?.trim();
        if (text && !text.includes('Connect to this network')) {
          const cleanText = text.replace(/\s+/g, ' ').replace(/:\s+/g, ': ');
          if (cleanText.includes(':')) content += `${cleanText}\n`;
        }
      });
    });

    // Use modern clipboard API if available, otherwise fallback
    if (navigator.clipboard && window.isSecureContext) {
      navigator.clipboard.writeText(content.trim()).then(() => {
        showCopySuccess(button);
      }).catch(err => {
        console.error('Copy failed:', err);
        fallbackCopy(content.trim(), button);
      });
    } else {
      // Fallback for non-HTTPS or older browsers
      fallbackCopy(content.trim(), button);
    }
  } catch (error) {
    console.error('Error copying section:', error);
  }
}

function copyConfigFile(button) {
  try {
    const configElement = document.getElementById('config-content');
    if (!configElement) throw new Error('Config content not found');

    const content = `=== Configuration File (config.json) ===\n\n${configElement.textContent.trim()}`;
    
    // Use modern clipboard API if available, otherwise fallback
    if (navigator.clipboard && window.isSecureContext) {
      navigator.clipboard.writeText(content).then(() => {
        showCopySuccess(button);
      }).catch(err => {
        console.error('Copy failed:', err);
        fallbackCopy(content, button);
      });
    } else {
      // Fallback for non-HTTPS or older browsers
      fallbackCopy(content, button);
    }
  } catch (error) {
    console.error('Error copying config:', error);
  }
}

function showCopySuccess(button) {
  const originalContent = button.innerHTML;
  button.innerHTML = '<span class="text-green-600">✓</span>';
  button.style.transform = 'scale(1.1)';
  setTimeout(() => {
    button.innerHTML = originalContent;
    button.style.transform = 'scale(1)';
  }, 1000);
}

function fallbackCopy(text, button) {
  const textarea = document.createElement('textarea');
  textarea.value = text;
  textarea.style.position = 'fixed';
  textarea.style.opacity = '0';
  document.body.appendChild(textarea);
  textarea.select();
  try {
    document.execCommand('copy');
    showCopySuccess(button);
  } catch (err) {
    console.error('Fallback copy failed:', err);
  }
  document.body.removeChild(textarea);
}

function updateProgressBar(elementId, percentage) {
  const element = document.querySelector(`[data-field="${elementId}"]`);
  if (element) {
    element.style.width = Math.min(Math.max(percentage, 0), 100) + '%';
    if (percentage > 90) element.classList.add('bg-red-500');
    else if (percentage > 75) element.classList.add('bg-orange-600');
    else element.classList.add('bg-orange-500');
  }
}

function setFieldValue(fieldName, value, isHtml = false) {
  const elements = document.querySelectorAll(`[data-field="${fieldName}"]`);
  elements.forEach(element => {
    if (isHtml) {
      element.innerHTML = value;
    } else {
      element.textContent = value || '-';
    }
  });
}

function formatBytes(bytes) {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  if (days > 0) return `${days}d ${hours}h ${minutes}m`;
  if (hours > 0) return `${hours}h ${minutes}m`;
  return `${minutes}m`;
}

function formatSignalStrength(dbm) {
  if (!dbm) return 'Unknown';
  
  let quality, color;
  
  if (dbm >= -30) {
    quality = 'Excellent';
    color = '#10b981'; // green
  } else if (dbm >= -50) {
    quality = 'Very Good';
    color = '#10b981'; // green
  } else if (dbm >= -60) {
    quality = 'Good';
    color = '#059669'; // darker green
  } else if (dbm >= -70) {
    quality = 'Fair';
    color = '#f59e0b'; // amber
  } else if (dbm >= -80) {
    quality = 'Weak';
    color = '#ef4444'; // red
  } else {
    quality = 'Very Weak';
    color = '#dc2626'; // darker red
  }
  
  return `${dbm} dBm (<span style="color: ${color}; font-weight: 600;">${quality}</span>)`;
}

function displayDiagnostics(data, configData) {
  // Device Configuration
  setFieldValue('device-owner', data.device?.owner);
  setFieldValue('timezone', data.device?.timezone);
  setFieldValue('mdns-hostname', data.device?.hostname);
  setFieldValue('max-message-chars', data.configuration?.max_message_chars);

  // Network
  setFieldValue('wifi-status', data.network?.wifi?.connected ? 'Connected' : 'Disconnected');
  setFieldValue('wifi-ssid', data.network?.wifi?.ssid);
  setFieldValue('ip-address', data.network?.wifi?.ip_address);
  setFieldValue('signal-strength', formatSignalStrength(data.network?.wifi?.signal_strength_dbm), true);
  setFieldValue('mac-address', data.network?.wifi?.mac_address);
  setFieldValue('gateway', data.network?.wifi?.gateway);
  setFieldValue('dns', data.network?.wifi?.dns);
  setFieldValue('wifi-connect-timeout', (data.network?.wifi?.connect_timeout_ms / 1000) + ' seconds');

  // MQTT
  setFieldValue('mqtt-enabled', data.network?.mqtt?.server ? 'Yes' : 'No');
  setFieldValue('mqtt-status', data.network?.mqtt?.connected ? 'Connected' : 'Disconnected');
  setFieldValue('mqtt-broker', data.network?.mqtt?.server);
  setFieldValue('mqtt-port', data.network?.mqtt?.port);
  setFieldValue('mqtt-topic', data.network?.mqtt?.topic);

  // Microcontroller
  setFieldValue('chip-model', data.hardware?.chip_model || 'Unknown');
  setFieldValue('cpu-frequency', data.hardware?.cpu_frequency_mhz + ' MHz');
  setFieldValue('flash-size', formatBytes(data.system?.flash?.total_chip_size));
  setFieldValue('firmware-version', data.hardware?.sdk_version);
  setFieldValue('uptime', formatUptime(data.system?.uptime_ms / 1000));
  setFieldValue('free-heap', formatBytes(data.system?.memory?.free_heap));
  setFieldValue('temperature', data.hardware?.temperature ? `${data.hardware.temperature.toFixed(1)}°C` : '-');

  // Memory usage
  const flashUsed = ((data.system?.flash?.app_partition?.used || 0) / (data.system?.flash?.app_partition?.total || 1)) * 100;
  const heapUsed = ((data.system?.memory?.used_heap || 0) / (data.system?.memory?.total_heap || 1)) * 100;
  setFieldValue('flash-usage-text', `${formatBytes(data.system?.flash?.app_partition?.used || 0)} / ${formatBytes(data.system?.flash?.app_partition?.total || 0)} (${flashUsed.toFixed(1)}%)`);
  setFieldValue('heap-usage-text', `${formatBytes(data.system?.memory?.used_heap || 0)} / ${formatBytes(data.system?.memory?.total_heap || 0)} (${heapUsed.toFixed(1)}%)`);
  updateProgressBar('flash-usage-bar', flashUsed);
  updateProgressBar('heap-usage-bar', heapUsed);

  // Unbidden Ink - using actual API structure
  setFieldValue('unbidden-ink-enabled', data.features?.unbidden_ink?.enabled ? 'Yes' : 'No');
  setFieldValue('unbidden-ink-interval', data.features?.unbidden_ink?.frequency_minutes + ' minutes');

  // Logging - using actual API structure
  setFieldValue('log-level', data.features?.logging?.level_name || 'Unknown');
  setFieldValue('serial-logging', data.features?.logging?.serial_enabled ? 'Enabled' : 'Disabled');
  setFieldValue('web-logging', data.features?.logging?.betterstack_enabled ? 'Enabled' : 'Disabled');
  setFieldValue('file-logging', data.features?.logging?.file_enabled ? 'Enabled' : 'Disabled');
  setFieldValue('mqtt-logging', data.features?.logging?.mqtt_enabled ? 'Enabled' : 'Disabled');

  // Hardware Buttons - improved layout with GPIO and configuration info
  const buttonsContainer = document.getElementById('buttons-content');
  if (buttonsContainer && data.features?.hardware_buttons) {
    buttonsContainer.innerHTML = ''; // Clear existing content
    
    const buttonsData = data.features.hardware_buttons;
    
    // Add general configuration section
    const configSection = document.createElement('div');
    configSection.className = 'border-b border-gray-200 dark:border-gray-600 pb-3 mb-3';
    
    const configTitle = document.createElement('h5');
    configTitle.className = 'text-sm font-semibold text-gray-700 dark:text-gray-300 mb-2';
    configTitle.textContent = 'Hardware Configuration';
    configSection.appendChild(configTitle);
    
    const configGrid = document.createElement('div');
    configGrid.className = 'grid grid-cols-2 gap-2 text-sm';
    
    const configItems = [
      ['Buttons', buttonsData.num_buttons || 'Unknown'],
      ['Debounce', `${buttonsData.debounce_ms || 'Unknown'} ms`],
      ['Long Press', `${buttonsData.long_press_ms || 'Unknown'} ms`],
      ['Active Low', buttonsData.active_low !== undefined ? (buttonsData.active_low ? 'Yes' : 'No') : 'Unknown'],
      ['Min Interval', `${buttonsData.min_interval_ms || 'Unknown'} ms`],
      ['Max/Minute', buttonsData.max_per_minute || 'Unknown']
    ];
    
    configItems.forEach(([label, value]) => {
      const item = document.createElement('div');
      item.innerHTML = `<span class="text-gray-600 dark:text-gray-400">${label}:</span> <span class="font-medium">${value}</span>`;
      configGrid.appendChild(item);
    });
    
    configSection.appendChild(configGrid);
    buttonsContainer.appendChild(configSection);
    
    // Add individual button sections
    if (buttonsData.buttons && buttonsData.buttons.length > 0) {
      buttonsData.buttons.forEach((button, index) => {
        const buttonSection = document.createElement('div');
        buttonSection.className = 'bg-gray-50 dark:bg-gray-800 rounded-lg p-3 border border-gray-200 dark:border-gray-600';
        
        const buttonTitle = document.createElement('h6');
        buttonTitle.className = 'text-sm font-semibold text-gray-700 dark:text-gray-300 mb-2';
        buttonTitle.textContent = `🔘 Button ${index + 1} (GPIO ${button.gpio || 'Unknown'})`;
        buttonSection.appendChild(buttonTitle);
        
        const buttonDetails = document.createElement('div');
        buttonDetails.className = 'space-y-1 text-sm';
        
        const addButtonDetail = (label, value) => {
          if (value && value !== 'Not configured') {
            const detail = document.createElement('div');
            detail.className = 'flex justify-between';
            detail.innerHTML = `
              <span class="text-gray-600 dark:text-gray-400">${label}:</span>
              <span class="font-medium text-gray-900 dark:text-gray-100">${value}</span>
            `;
            buttonDetails.appendChild(detail);
          }
        };
        
        addButtonDetail('Short Press', button.short_endpoint || 'Not configured');
        addButtonDetail('Long Press', button.long_endpoint || 'Not configured');
        if (button.short_mqtt_topic) addButtonDetail('Short MQTT', button.short_mqtt_topic);
        if (button.long_mqtt_topic) addButtonDetail('Long MQTT', button.long_mqtt_topic);
        
        buttonSection.appendChild(buttonDetails);
        buttonsContainer.appendChild(buttonSection);
      });
    } else {
      const noButtonsMsg = document.createElement('div');
      noButtonsMsg.className = 'text-gray-500 dark:text-gray-400 text-center py-4';
      noButtonsMsg.textContent = 'No button configuration found';
      buttonsContainer.appendChild(noButtonsMsg);
    }
  }

  // Pages & Endpoints - improved layout
  const webPagesContainer = document.getElementById('web-pages');
  const apiEndpointsContainer = document.getElementById('api-endpoints');
  
  if (data.endpoints?.web_pages && webPagesContainer) {
    webPagesContainer.innerHTML = '';
    data.endpoints.web_pages.forEach(page => {
      const pageItem = document.createElement('div');
      pageItem.className = 'bg-blue-50 dark:bg-blue-900/20 rounded-lg p-3 border border-blue-200 dark:border-blue-800';
      
      const pathElement = document.createElement('div');
      pathElement.className = 'font-mono text-sm font-semibold text-blue-700 dark:text-blue-300';
      pathElement.textContent = page.path;
      
      const descElement = document.createElement('div');
      descElement.className = 'text-sm text-blue-600 dark:text-blue-400 mt-1';
      descElement.textContent = page.description;
      
      pageItem.appendChild(pathElement);
      pageItem.appendChild(descElement);
      webPagesContainer.appendChild(pageItem);
    });
  } else if (webPagesContainer) {
    webPagesContainer.innerHTML = '<div class="text-gray-500">Loading...</div>';
  }
  
  if (data.endpoints?.api_endpoints && apiEndpointsContainer) {
    apiEndpointsContainer.innerHTML = '';
    
    // Group endpoints by method
    const groupedEndpoints = {};
    data.endpoints.api_endpoints.forEach(endpoint => {
      if (!groupedEndpoints[endpoint.method]) {
        groupedEndpoints[endpoint.method] = [];
      }
      groupedEndpoints[endpoint.method].push(endpoint);
    });
    
    // Display grouped endpoints
    Object.entries(groupedEndpoints).forEach(([method, endpoints]) => {
      const methodSection = document.createElement('div');
      methodSection.className = 'mb-4';
      
      const methodTitle = document.createElement('h6');
      methodTitle.className = 'text-sm font-semibold text-gray-700 dark:text-gray-300 mb-2';
      methodTitle.textContent = `${method} Endpoints (${endpoints.length})`;
      methodSection.appendChild(methodTitle);
      
      const endpointsList = document.createElement('div');
      endpointsList.className = 'space-y-2';
      
      endpoints.forEach(endpoint => {
        const endpointItem = document.createElement('div');
        endpointItem.className = 'bg-gray-50 dark:bg-gray-800 rounded-lg p-3 border border-gray-200 dark:border-gray-600';
        
        const pathElement = document.createElement('div');
        pathElement.className = 'font-mono text-sm font-semibold text-gray-900 dark:text-gray-100';
        pathElement.textContent = endpoint.path;
        
        const descElement = document.createElement('div');
        descElement.className = 'text-sm text-gray-600 dark:text-gray-400 mt-1';
        descElement.textContent = endpoint.description;
        
        endpointItem.appendChild(pathElement);
        endpointItem.appendChild(descElement);
        endpointsList.appendChild(endpointItem);
      });
      
      methodSection.appendChild(endpointsList);
      apiEndpointsContainer.appendChild(methodSection);
    });
  } else if (apiEndpointsContainer) {
    apiEndpointsContainer.innerHTML = '<div class="text-gray-500">Loading...</div>';
  }

  // Configuration File - loaded from /api/config
  const configElement = document.getElementById('config-content');
  if (configData && configElement) {
    const redactedConfig = redactSecrets(configData);
    configElement.textContent = JSON.stringify(redactedConfig, null, 2);
  } else if (configElement) {
    configElement.textContent = 'Configuration not available';
  }
}

async function loadDiagnostics() {
  const loadingElement = document.getElementById('loading');
  const errorElement = document.getElementById('error');
  const contentElement = document.getElementById('diagnostics-content');

  loadingElement.style.display = 'block';
  errorElement.classList.remove('show');
  contentElement.style.display = 'none';

  try {
    // Wait for global config to be loaded (avoiding duplicate /api/config calls)
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

    // Load only diagnostics data (config already loaded globally)
    const response = await fetch('/api/diagnostics');
    if (!response.ok) {
      throw new Error(`Diagnostics API error: HTTP ${response.status}: ${response.statusText}`);
    }
    const diagnosticsData = await response.json();

    // Use the already-loaded global config
    displayDiagnostics(diagnosticsData, window.GLOBAL_CONFIG);
    loadingElement.style.display = 'none';
    contentElement.style.display = 'block';
    showSection(currentSection);
  } catch (error) {
    console.error('Error loading diagnostics:', error);
    loadingElement.style.display = 'none';
    errorElement.classList.add('show');
    document.getElementById('error-message').textContent = error.message;
  }
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
  loadDiagnostics();
  showSection('device-config-section');
  
  // Auto-refresh every 30 seconds
  setInterval(loadDiagnostics, 30000);
});
