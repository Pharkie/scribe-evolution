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
    btn.classList.remove('font-bold', 'ring-2');
  });
  
  const section = document.getElementById(sectionId);
  if (section) {
    section.classList.add('show');
    currentSection = sectionId;
  }
  
  const activeBtn = event?.target;
  if (activeBtn) {
    activeBtn.classList.add('font-bold', 'ring-2');
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
        if (labelText && valueText && valueText !== '-') {
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

    navigator.clipboard.writeText(content.trim()).then(() => {
      showCopySuccess(button);
    }).catch(err => {
      console.error('Copy failed:', err);
      fallbackCopy(content.trim(), button);
    });
  } catch (error) {
    console.error('Error copying section:', error);
  }
}

function copyConfigFile(button) {
  try {
    const configElement = document.getElementById('config-content');
    if (!configElement) throw new Error('Config content not found');

    const content = `=== Configuration File (config.json) ===\n\n${configElement.textContent.trim()}`;
    
    navigator.clipboard.writeText(content).then(() => {
      showCopySuccess(button);
    }).catch(err => {
      console.error('Copy failed:', err);
      fallbackCopy(content, button);
    });
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

function displayDiagnostics(data) {
  // Device Configuration
  setFieldValue('device-owner', data.device_owner);
  setFieldValue('timezone', data.timezone);
  setFieldValue('mdns-hostname', data.mdns_hostname);
  setFieldValue('max-message-chars', data.max_message_chars);

  // Network
  setFieldValue('wifi-status', data.wifi_status);
  setFieldValue('wifi-ssid', data.wifi_ssid);
  setFieldValue('ip-address', data.ip_address);
  setFieldValue('signal-strength', data.signal_strength);
  setFieldValue('mac-address', data.mac_address);
  setFieldValue('gateway', data.gateway);
  setFieldValue('dns', data.dns);
  setFieldValue('wifi-connect-timeout', data.wifi_connect_timeout + ' seconds');

  // MQTT
  setFieldValue('mqtt-enabled', data.mqtt_enabled ? 'Yes' : 'No');
  setFieldValue('mqtt-status', data.mqtt_status);
  setFieldValue('mqtt-broker', data.mqtt_broker);
  setFieldValue('mqtt-port', data.mqtt_port);
  setFieldValue('mqtt-username', data.mqtt_username);
  setFieldValue('mqtt-topic', data.mqtt_topic);
  setFieldValue('mqtt-keep-alive', data.mqtt_keep_alive + ' seconds');

  // Microcontroller
  setFieldValue('cpu-frequency', data.cpu_frequency + ' MHz');
  setFieldValue('flash-size', formatBytes(data.flash_size));
  setFieldValue('firmware-version', data.firmware_version);
  setFieldValue('uptime', formatUptime(data.uptime));
  setFieldValue('free-heap', formatBytes(data.free_heap));
  setFieldValue('temperature', data.temperature + '°C');

  // Memory usage
  const flashUsed = ((data.flash_size - data.flash_free) / data.flash_size) * 100;
  const heapUsed = ((data.total_heap - data.free_heap) / data.total_heap) * 100;
  setFieldValue('flash-usage-text', `${formatBytes(data.flash_size - data.flash_free)} / ${formatBytes(data.flash_size)} (${flashUsed.toFixed(1)}%)`);
  setFieldValue('heap-usage-text', `${formatBytes(data.total_heap - data.free_heap)} / ${formatBytes(data.total_heap)} (${heapUsed.toFixed(1)}%)`);
  updateProgressBar('flash-usage-bar', flashUsed);
  updateProgressBar('heap-usage-bar', heapUsed);

  // Unbidden Ink
  setFieldValue('unbidden-ink-enabled', data.unbidden_ink_enabled ? 'Yes' : 'No');
  setFieldValue('unbidden-ink-interval', data.unbidden_ink_interval);
  setFieldValue('unbidden-ink-last-run', data.unbidden_ink_last_run || 'Never');
  setFieldValue('unbidden-ink-next-run', data.unbidden_ink_next_run || 'Not scheduled');
  setFieldValue('unbidden-ink-total-riddles', data.unbidden_ink_total_riddles);
  setFieldValue('unbidden-ink-current-index', data.unbidden_ink_current_index);

  // Logging
  setFieldValue('log-level', data.log_level);
  setFieldValue('serial-logging', data.serial_logging ? 'Enabled' : 'Disabled');
  setFieldValue('web-logging', data.web_logging ? 'Enabled' : 'Disabled');
  setFieldValue('file-logging', data.file_logging ? 'Enabled' : 'Disabled');
  setFieldValue('log-buffer-size', data.log_buffer_size);

  // Hardware Buttons
  setFieldValue('button-a-pin', data.button_a_pin || 'Not configured');
  setFieldValue('button-b-pin', data.button_b_pin || 'Not configured');
  setFieldValue('button-c-pin', data.button_c_pin || 'Not configured');
  setFieldValue('button-a-state', data.button_a_state || 'Unknown');
  setFieldValue('button-b-state', data.button_b_state || 'Unknown');
  setFieldValue('button-c-state', data.button_c_state || 'Unknown');

  // Pages & Endpoints
  const webPagesContainer = document.getElementById('web-pages');
  const apiEndpointsContainer = document.getElementById('api-endpoints');
  
  if (data.web_pages && webPagesContainer) {
    webPagesContainer.innerHTML = data.web_pages.map(page => 
      `<div class="flex justify-between"><span>${escapeHtml(page.path)}</span><span class="text-slate-600 dark:text-slate-400">${escapeHtml(page.description)}</span></div>`
    ).join('');
  }
  
  if (data.api_endpoints && apiEndpointsContainer) {
    apiEndpointsContainer.innerHTML = data.api_endpoints.map(endpoint => 
      `<div class="flex justify-between"><span class="font-mono text-sm">${escapeHtml(endpoint.method)} ${escapeHtml(endpoint.path)}</span><span class="text-slate-600 dark:text-slate-400">${escapeHtml(endpoint.description)}</span></div>`
    ).join('');
  }

  // Configuration File
  const configElement = document.getElementById('config-content');
  if (data.config_file && configElement) {
    const redactedConfig = redactSecrets(data.config_file);
    configElement.textContent = JSON.stringify(redactedConfig, null, 2);
  }
}

function loadDiagnostics() {
  const loadingElement = document.getElementById('loading');
  const errorElement = document.getElementById('error');
  const contentElement = document.getElementById('diagnostics-content');

  loadingElement.style.display = 'block';
  errorElement.classList.remove('show');
  contentElement.style.display = 'none';

  fetch('/api/diagnostics')
    .then(response => {
      if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      return response.json();
    })
    .then(data => {
      displayDiagnostics(data);
      loadingElement.style.display = 'none';
      contentElement.style.display = 'block';
      showSection(currentSection);
    })
    .catch(error => {
      console.error('Error loading diagnostics:', error);
      loadingElement.style.display = 'none';
      errorElement.classList.add('show');
      document.getElementById('error-message').textContent = error.message;
    });
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
  loadDiagnostics();
  showSection('device-config-section');
  
  // Auto-refresh every 30 seconds
  setInterval(loadDiagnostics, 30000);
});
