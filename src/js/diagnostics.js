// System diagnostics and status display functionality

/**
 * Initialize diagnostics page when DOM is loaded
 */
document.addEventListener('DOMContentLoaded', function() {
  setupDiagnosticsEventListeners();
  loadDiagnostics();
  showSection('device-config-section');
});

/**
 * Setup event listeners for diagnostics page
 */
function setupDiagnosticsEventListeners() {
  // Section navigation buttons
  document.querySelectorAll('[data-section]').forEach(button => {
    button.addEventListener('click', function() {
      const sectionId = this.dataset.section;
      showSection(sectionId);
    });
  });
  
  // Generic copy buttons
  document.querySelectorAll('.generic-copy-btn').forEach(button => {
    button.addEventListener('click', function() {
      const sectionName = this.dataset.sectionName;
      copyGenericSection(sectionName, this);
    });
  });

  // Quick action buttons (for character test)
  document.querySelectorAll('.quick-action-btn').forEach(button => {
    button.addEventListener('click', function() {
      const action = this.dataset.action;
      if (action) {
        handleQuickAction(action);
      }
    });
  });
  
  // Config copy button
  const configCopyBtn = document.getElementById('config-copy-btn');
  if (configCopyBtn) {
    configCopyBtn.addEventListener('click', function() {
      copyConfigFile(this);
    });
  }
}

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
  
  // Find and activate the button that corresponds to this section
  const activeBtn = document.querySelector(`[data-section="${sectionId}"]`);
  if (activeBtn && activeBtn.classList) {
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

// JSON syntax highlighting function
function highlightJSON(json) {
  return json
    .replace(/("(\\u[a-zA-Z0-9]{4}|\\[^u]|[^\\"])*"(\s*:)?|\b(true|false|null)\b|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)/g, function (match) {
      let cls = 'json-number';
      if (/^"/.test(match)) {
        if (/:$/.test(match)) {
          cls = 'json-key';
        } else {
          cls = 'json-string';
        }
      } else if (/true|false/.test(match)) {
        cls = 'json-boolean';
      } else if (/null/.test(match)) {
        cls = 'json-null';
      }
      return '<span class="' + cls + '">' + match + '</span>';
    })
    .replace(/([{}[\],])/g, '<span class="json-punctuation">$1</span>');
}

function copyConfigFile(button) {
  try {
    const configElement = document.getElementById('config-content');
    if (!configElement) throw new Error('Config content not found');

    // Get plain text content for copying (strip HTML if present)
    const content = `=== Runtime Configuration (NVS) ===\n\n${configElement.textContent.trim()}`;
    
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
  return parseFloat((bytes / Math.pow(k, i)).toFixed(0)) + ' ' + sizes[i];
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

function formatTemperature(tempC) {
  if (!tempC || isNaN(tempC)) return '-';
  
  let status, color;
  
  if (tempC < 35) {
    status = 'Cool';
    color = '#3b82f6'; // blue
  } else if (tempC < 50) {
    status = 'Normal';
    color = '#10b981'; // green
  } else if (tempC < 65) {
    status = 'Warm';
    color = '#f59e0b'; // amber
  } else if (tempC < 80) {
    status = 'Hot';
    color = '#ef4444'; // red
  } else {
    status = 'Critical';
    color = '#dc2626'; // darker red
  }
  
  return `${tempC.toFixed(1)}°C (<span style="color: ${color}; font-weight: 600;">${status}</span>)`;
}

function displayDiagnostics(data, configData) {
  // Device Configuration
  setFieldValue('device-owner', data.device?.owner);
  setFieldValue('timezone', data.device?.timezone);
  setFieldValue('mdns-hostname', data.device?.hostname);
  setFieldValue('max-message-chars', configData?.validation?.maxCharacters);

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
  setFieldValue('temperature', formatTemperature(data.hardware?.temperature), true);

  // Memory usage
  const flashUsed = ((data.system?.flash?.app_partition?.used || 0) / (data.system?.flash?.app_partition?.total || 1)) * 100;
  const heapUsed = ((data.system?.memory?.used_heap || 0) / (data.system?.memory?.total_heap || 1)) * 100;
  setFieldValue('flash-usage-text', `${formatBytes(data.system?.flash?.app_partition?.used || 0)} / ${formatBytes(data.system?.flash?.app_partition?.total || 0)} (${flashUsed.toFixed(0)}%)`);
  setFieldValue('heap-usage-text', `${formatBytes(data.system?.memory?.used_heap || 0)} / ${formatBytes(data.system?.memory?.total_heap || 0)} (${heapUsed.toFixed(0)}%)`);
  updateProgressBar('flash-usage-bar', flashUsed);
  updateProgressBar('heap-usage-bar', heapUsed);

  // Unbidden Ink - using actual API structure
  const unbiddenInk = data.features?.unbidden_ink;
  setFieldValue('unbidden-ink-enabled', unbiddenInk?.enabled ? 'Yes' : 'No');
  setFieldValue('unbidden-ink-start-hour', unbiddenInk?.start_hour !== undefined ? `${String(unbiddenInk.start_hour).padStart(2, '0')}:00` : 'Unknown');
  setFieldValue('unbidden-ink-end-hour', unbiddenInk?.end_hour !== undefined ? `${String(unbiddenInk.end_hour).padStart(2, '0')}:00` : 'Unknown');
  setFieldValue('unbidden-ink-frequency', unbiddenInk?.frequency_minutes !== undefined ? `${unbiddenInk.frequency_minutes} minutes` : 'Unknown');
  
  // Format next message time
  if (unbiddenInk?.next_message_time && unbiddenInk.next_message_time > 0) {
    // next_message_time is in milliseconds from device boot (millis())
    // We need to calculate the relative time from now
    const currentDeviceTime = Date.now(); // Current time in ms since epoch
    const deviceUptimeMs = unbiddenInk.next_message_time; // Device uptime when message is scheduled
    
    // We can't directly convert device uptime to real time without knowing device boot time
    // So let's treat it as "time remaining from now" and use the system uptime from diagnostics
    const uptimeMs = data.microcontroller?.uptime_ms || 0;
    const timeRemainingMs = deviceUptimeMs - uptimeMs;
    
    if (timeRemainingMs <= 0) {
      setFieldValue('unbidden-ink-next-message', 'Overdue');
    } else {
      const seconds = Math.floor(timeRemainingMs / 1000);
      const minutes = Math.floor(seconds / 60);
      const hours = Math.floor(minutes / 60);
      
      let timeText;
      if (seconds <= 30) {
        timeText = 'Imminently';
      } else if (minutes < 1) {
        timeText = '1 min';
      } else if (minutes < 60) {
        timeText = `${minutes} mins`;
      } else if (hours === 1) {
        timeText = 'About an hour';
      } else if (hours < 3) {
        const halfHours = Math.round(hours * 2) / 2;
        timeText = `About ${halfHours} hours`;
      } else {
        const roundedHours = Math.round(hours * 2) / 2;
        timeText = `About ${roundedHours} hours`;
      }
      
      setFieldValue('unbidden-ink-next-message', timeText);
    }
  } else {
    setFieldValue('unbidden-ink-next-message', unbiddenInk?.enabled ? 'Unknown' : 'Disabled');
  }

  // Logging - using actual API structure
  setFieldValue('log-level', data.features?.logging?.level_name || 'Unknown');
  setFieldValue('serial-logging', data.features?.logging?.serial_enabled ? 'Enabled' : 'Disabled');
  setFieldValue('web-logging', data.features?.logging?.betterstack_enabled ? 'Enabled' : 'Disabled');
  setFieldValue('file-logging', data.features?.logging?.file_enabled ? 'Enabled' : 'Disabled');
  setFieldValue('mqtt-logging', data.features?.logging?.mqtt_enabled ? 'Enabled' : 'Disabled');

  // Hardware Buttons - hybrid approach: static HTML structure with dynamic content
  if (data.features?.hardware_buttons) {
    const buttonsData = data.features.hardware_buttons;
    
    // Update hardware configuration values
    setFieldValue('hw-buttons-count', buttonsData.num_buttons || 'Unknown');
    setFieldValue('hw-buttons-debounce', `${buttonsData.debounce_ms || 'Unknown'} ms`);
    setFieldValue('hw-buttons-long-press', `${buttonsData.long_press_ms || 'Unknown'} ms`);
    setFieldValue('hw-buttons-active-low', buttonsData.active_low !== undefined ? (buttonsData.active_low ? 'Yes' : 'No') : 'Unknown');
    setFieldValue('hw-buttons-min-interval', `${buttonsData.min_interval_ms || 'Unknown'} ms`);
    setFieldValue('hw-buttons-max-per-minute', buttonsData.max_per_minute || 'Unknown');
    
    // Update individual button configurations
    if (buttonsData.buttons && buttonsData.buttons.length > 0) {
      buttonsData.buttons.forEach((button, index) => {
        const buttonNum = index + 1;
        
        // Update button title with GPIO info
        setFieldValue(`button-${buttonNum}-title`, `Button ${buttonNum} (GPIO ${button.gpio || 'Unknown'})`);
        
        // Update button actions
        setFieldValue(`button-${buttonNum}-short-press`, button.short_endpoint || 'Not configured');
        setFieldValue(`button-${buttonNum}-long-press`, button.long_endpoint || 'Not configured');
        
        // Handle optional MQTT topics - show/hide rows as needed
        const shortMqttContainer = document.querySelector(`[data-field-container="button-${buttonNum}-short-mqtt"]`);
        const longMqttContainer = document.querySelector(`[data-field-container="button-${buttonNum}-long-mqtt"]`);
        
        if (button.short_mqtt_topic) {
          setFieldValue(`button-${buttonNum}-short-mqtt`, button.short_mqtt_topic);
          if (shortMqttContainer) shortMqttContainer.style.display = 'flex';
        } else {
          if (shortMqttContainer) shortMqttContainer.style.display = 'none';
        }
        
        if (button.long_mqtt_topic) {
          setFieldValue(`button-${buttonNum}-long-mqtt`, button.long_mqtt_topic);
          if (longMqttContainer) longMqttContainer.style.display = 'flex';
        } else {
          if (longMqttContainer) longMqttContainer.style.display = 'none';
        }
      });
      
      // Hide unused button sections
      for (let i = buttonsData.buttons.length + 1; i <= 4; i++) {
        const buttonSection = document.querySelector(`[data-button="${i}"]`);
        if (buttonSection) {
          buttonSection.style.display = 'none';
        }
      }
      
      // Show used button sections
      for (let i = 1; i <= buttonsData.buttons.length; i++) {
        const buttonSection = document.querySelector(`[data-button="${i}"]`);
        if (buttonSection) {
          buttonSection.style.display = 'block';
        }
      }
    } else {
      // Hide all button sections if no buttons configured
      for (let i = 1; i <= 4; i++) {
        const buttonSection = document.querySelector(`[data-button="${i}"]`);
        if (buttonSection) {
          buttonSection.style.display = 'none';
        }
      }
    }
  }

  // Pages & Endpoints - hybrid approach: static HTML structure with dynamic content
  const webPagesContainer = document.getElementById('web-pages');
  const apiEndpointsContainer = document.getElementById('api-endpoints');
  
  if (data.endpoints?.web_pages && webPagesContainer) {
    // Clear existing content including loading placeholder
    webPagesContainer.innerHTML = '';
    
    // Create structured rows for each web page
    data.endpoints.web_pages.forEach(page => {
      const pageRow = document.createElement('div');
      pageRow.className = 'diag-row';
      
      const pathLabel = document.createElement('span');
      pathLabel.className = 'diag-label';
      pathLabel.textContent = page.path;
      
      const descValue = document.createElement('span');
      descValue.className = 'diag-value';
      descValue.textContent = page.description;
      
      pageRow.appendChild(pathLabel);
      pageRow.appendChild(descValue);
      webPagesContainer.appendChild(pageRow);
    });
  } else if (webPagesContainer) {
    // Keep loading state with proper structure
    const loadingRow = webPagesContainer.querySelector('.loading-placeholder');
    if (loadingRow) {
      loadingRow.querySelector('.diag-label').textContent = 'Loading web pages...';
      loadingRow.querySelector('.diag-value').textContent = '-';
    }
  }
  
  if (data.endpoints?.api_endpoints && apiEndpointsContainer) {
    // Clear existing content including loading placeholder
    apiEndpointsContainer.innerHTML = '';
    
    // Group endpoints by method
    const groupedEndpoints = {};
    data.endpoints.api_endpoints.forEach(endpoint => {
      if (!groupedEndpoints[endpoint.method]) {
        groupedEndpoints[endpoint.method] = [];
      }
      groupedEndpoints[endpoint.method].push(endpoint);
    });
    
    // Create structured rows for each endpoint group
    Object.entries(groupedEndpoints).forEach(([method, endpoints]) => {
      // Method header row
      const methodRow = document.createElement('div');
      methodRow.className = 'diag-row method-header';
      
      const methodLabel = document.createElement('span');
      methodLabel.className = 'diag-label font-semibold text-gray-700 dark:text-gray-300';
      methodLabel.textContent = `${method} Endpoints`;
      
      const methodCount = document.createElement('span');
      methodCount.className = 'diag-value font-semibold text-gray-700 dark:text-gray-300';
      methodCount.textContent = `(${endpoints.length})`;
      
      methodRow.appendChild(methodLabel);
      methodRow.appendChild(methodCount);
      apiEndpointsContainer.appendChild(methodRow);
      
      // Individual endpoint rows
      endpoints.forEach(endpoint => {
        const endpointRow = document.createElement('div');
        endpointRow.className = 'diag-row ml-4';
        
        const pathLabel = document.createElement('span');
        pathLabel.className = 'diag-label font-mono text-sm';
        pathLabel.textContent = endpoint.path;
        
        const descValue = document.createElement('span');
        descValue.className = 'diag-value text-sm';
        descValue.textContent = endpoint.description;
        
        endpointRow.appendChild(pathLabel);
        endpointRow.appendChild(descValue);
        apiEndpointsContainer.appendChild(endpointRow);
      });
    });
  } else if (apiEndpointsContainer) {
    // Keep loading state with proper structure
    const loadingRow = apiEndpointsContainer.querySelector('.loading-placeholder');
    if (loadingRow) {
      loadingRow.querySelector('.diag-label').textContent = 'Loading API endpoints...';
      loadingRow.querySelector('.diag-value').textContent = '-';
    }
  }

  // Configuration File - loaded from /api/config
  const configElement = document.getElementById('config-content');
  if (configData && configElement) {
    const redactedConfig = redactSecrets(configData);
    const jsonString = JSON.stringify(redactedConfig, null, 2);
    configElement.innerHTML = highlightJSON(jsonString);
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
    // Load both diagnostics and config data in parallel
    const [diagnosticsResponse, configResponse] = await Promise.all([
      fetch('/api/diagnostics'),
      fetch('/api/config')
    ]);
    
    if (!diagnosticsResponse.ok) {
      throw new Error(`Diagnostics API error: HTTP ${diagnosticsResponse.status}: ${diagnosticsResponse.statusText}`);
    }
    
    if (!configResponse.ok) {
      throw new Error(`Config API error: HTTP ${configResponse.status}: ${configResponse.statusText}`);
    }
    
    const diagnosticsData = await diagnosticsResponse.json();
    const configData = await configResponse.json();

    displayDiagnostics(diagnosticsData, configData);
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

/**
 * Handle quick action button clicks (for character test)
 */
async function handleQuickAction(action) {
  try {
    // Step 1: Generate content from the action endpoint
    let endpoint = `/api/${action}`;
    
    const response = await fetch(endpoint, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      }
    });

    if (!response.ok) {
      const errorData = await response.text();
      showToast(`Error generating content: ${errorData}`, 'error');
      return;
    }

    const contentResult = await response.json();
    
    if (!contentResult.content) {
      showToast('No content received from server', 'error');
      return;
    }

    // Step 2: Print the content locally using /api/print-local
    const printResponse = await fetch('/api/print-local', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ message: contentResult.content })
    });

    if (printResponse.ok) {
      showToast(`${action} sent to printer successfully!`, 'success');
    } else {
      const errorData = await printResponse.text();
      showToast(`Print error: ${errorData}`, 'error');
    }
  } catch (error) {
    console.error('Error sending quick action:', error);
    showToast(`Network error: ${error.message}`, 'error');
  }
}

/**
 * Show toast notification
 */
function showToast(message, type = 'info') {
  // Simple toast implementation for diagnostics page
  const toast = document.createElement('div');
  toast.textContent = message;
  toast.className = `fixed top-4 right-4 px-4 py-2 rounded-lg text-white z-50 ${
    type === 'success' ? 'bg-green-500' : 
    type === 'error' ? 'bg-red-500' : 'bg-blue-500'
  }`;
  
  document.body.appendChild(toast);
  
  setTimeout(() => {
    toast.remove();
  }, 3000);
}


