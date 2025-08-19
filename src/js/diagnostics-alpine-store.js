/**
 * @file diagnostics-alpine-store.js
 * @brief Alpine.js reactive store for diagnostics page functionality
 */

// Initialize Alpine.js diagnostics store with data loading
function initializeDiagnosticsStore() {
  const store = {
    // Core state
    loading: true,
    error: null,
    initialized: false, // Flag to prevent duplicate initialization
    diagnosticsData: {},
    configData: {},
    nvsData: {},
    
    // UI state
    currentSection: 'device-config-section',
    
    // Section definitions
    sections: [
      { id: 'device-config-section', name: 'Device', icon: '⚙️', color: 'purple' },
      { id: 'network-section', name: 'Network', icon: '🌐', color: 'blue' },
      { id: 'mqtt-section', name: 'MQTT', icon: '📡', color: 'green' },
      { id: 'microcontroller-section', name: 'Microcontroller', icon: '🎛️', color: 'orange' },
      { id: 'unbidden-ink-section', name: 'Unbidden Ink', icon: '🎲', color: 'yellow' },
      { id: 'logging-section', name: 'Logging', icon: '📋', color: 'indigo' },
      { id: 'hardware-buttons-section', name: 'Buttons', icon: '🎛️', color: 'pink' },
      { id: 'pages-endpoints-section', name: 'Pages & Endpoints', icon: '🔗', color: 'teal' },
      { id: 'config-file-section', name: 'Runtime Configuration', icon: '⚙️', color: 'red' },
      { id: 'nvs-storage-section', name: 'NVS', icon: '💾', color: 'cyan' }
    ],
    
    // Initialize store
    async init() {
      // Prevent duplicate initialization
      if (this.initialized) {
        console.log('🛠️ Diagnostics: Already initialized, skipping');
        return;
      }
      this.initialized = true;
      
      await this.loadDiagnostics();
    },
    
        // Load all diagnostics data
    async loadDiagnostics() {
      this.loading = true;
      this.error = null;
      
      try {
        // Load diagnostics, config, and NVS data in parallel with individual error handling
        const responses = await Promise.allSettled([
          fetch('/api/diagnostics'),
          fetch('/api/config'),
          fetch('/api/nvs-dump')
        ]);
        
        // Handle diagnostics response
        if (responses[0].status === 'fulfilled' && responses[0].value.ok) {
          this.diagnosticsData = await responses[0].value.json();
        } else {
          console.warn('Failed to load diagnostics data:', responses[0].reason || responses[0].value.statusText);
          this.diagnosticsData = {};
        }
        
        // Handle config response
        if (responses[1].status === 'fulfilled' && responses[1].value.ok) {
          this.configData = await responses[1].value.json();
        } else {
          console.warn('Failed to load config data:', responses[1].reason || responses[1].value.statusText);
          this.configData = {};
        }
        
        // Handle NVS dump response
        if (responses[2].status === 'fulfilled' && responses[2].value.ok) {
          this.nvsData = await responses[2].value.json();
        } else {
          console.warn('Failed to load NVS data:', responses[2].reason || responses[2].value.statusText);
          this.nvsData = {};
        }
        
        this.loading = false;
      } catch (error) {
        console.error('Error loading diagnostics:', error);
        this.error = error.message;
        this.loading = false;
      }
    },
    
    // Section management
    showSection(sectionId) {
      this.currentSection = sectionId;
    },
    
    getSectionClass(sectionId) {
      const section = this.sections.find(s => s.id === sectionId);
      const baseClass = 'section-nav-btn';
      const colorClass = `section-nav-btn-${section?.color || 'purple'}`;
      const activeClass = this.currentSection === sectionId ? 'active' : '';
      return `${baseClass} ${colorClass} ${activeClass}`.trim();
    },
    
    // Device Configuration computed properties
    get deviceConfig() {
      return {
        owner: this.diagnosticsData.device?.owner || '-',
        timezone: this.diagnosticsData.device?.timezone || '-',
        mdnsHostname: this.diagnosticsData.device?.hostname || '-',
        maxMessageChars: this.configData?.validation?.maxCharacters || '-'
      };
    },
    
    // Network computed properties
    get networkInfo() {
      const wifi = this.diagnosticsData.network?.wifi || {};
      return {
        wifiStatus: wifi.connected ? 'Connected' : 'Disconnected',
        ssid: wifi.ssid || '-',
        ipAddress: wifi.ip_address || '-',
        signalStrength: this.formatSignalStrength(wifi.signal_strength_dbm),
        macAddress: wifi.mac_address || '-',
        gateway: wifi.gateway || '-',
        dns: wifi.dns || '-',
        connectTimeout: wifi.connect_timeout_ms ? `${wifi.connect_timeout_ms / 1000} seconds` : '-'
      };
    },
    
    // MQTT computed properties
    get mqttInfo() {
      const mqtt = this.diagnosticsData.network?.mqtt || {};
      return {
        enabled: mqtt.server ? 'Yes' : 'No',
        status: mqtt.connected ? 'Connected' : 'Disconnected',
        broker: mqtt.server || '-',
        port: mqtt.port || '-',
        topic: mqtt.topic || '-'
      };
    },
    
    // Microcontroller computed properties
    get microcontrollerInfo() {
      const hardware = this.diagnosticsData.hardware || {};
      const system = this.diagnosticsData.system || {};
      
      return {
        chipModel: hardware.chip_model || 'Unknown',
        cpuFrequency: hardware.cpu_frequency_mhz ? `${hardware.cpu_frequency_mhz} MHz` : '-',
        flashSize: this.formatBytes(system.flash?.total_chip_size),
        firmwareVersion: hardware.sdk_version || '-',
        uptime: this.formatUptime(system.uptime_ms / 1000),
        temperature: this.formatTemperature(hardware.temperature)
      };
    },
    
    // Memory usage computed properties
    get memoryUsage() {
      const system = this.diagnosticsData.system || {};
      const flash = system.flash?.app_partition || {};
      const memory = system.memory || {};
      
      const flashUsed = flash.total ? ((flash.used || 0) / flash.total) * 100 : 0;
      const heapUsed = memory.total_heap ? ((memory.used_heap || 0) / memory.total_heap) * 100 : 0;
      
      return {
        flashUsageText: `${this.formatBytes(flash.used || 0)} / ${this.formatBytes(flash.total || 0)} (${flashUsed.toFixed(0)}%)`,
        heapUsageText: `${this.formatBytes(memory.used_heap || 0)} / ${this.formatBytes(memory.total_heap || 0)} (${heapUsed.toFixed(0)}%)`,
        flashUsagePercent: flashUsed,
        heapUsagePercent: heapUsed
      };
    },
    
    // Unbidden Ink computed properties
    get unbiddenInkInfo() {
      const unbiddenInk = this.diagnosticsData.features?.unbidden_ink || {};
      
      let nextMessageText = 'Unknown';
      if (unbiddenInk.next_message_time && unbiddenInk.next_message_time > 0) {
        const uptimeMs = this.diagnosticsData.microcontroller?.uptime_ms || 0;
        const timeRemainingMs = unbiddenInk.next_message_time - uptimeMs;
        
        if (timeRemainingMs <= 0) {
          nextMessageText = 'Overdue';
        } else {
          const seconds = Math.floor(timeRemainingMs / 1000);
          const minutes = Math.floor(seconds / 60);
          const hours = Math.floor(minutes / 60);
          
          if (seconds <= 30) {
            nextMessageText = 'Imminently';
          } else if (minutes < 1) {
            nextMessageText = '1 min';
          } else if (minutes < 60) {
            nextMessageText = `${minutes} mins`;
          } else if (hours === 1) {
            nextMessageText = 'About an hour';
          } else if (hours < 3) {
            const halfHours = Math.round(hours * 2) / 2;
            nextMessageText = `About ${halfHours} hours`;
          } else {
            const roundedHours = Math.round(hours * 2) / 2;
            nextMessageText = `About ${roundedHours} hours`;
          }
        }
      } else {
        nextMessageText = unbiddenInk.enabled ? 'Unknown' : 'Disabled';
      }
      
      return {
        enabled: unbiddenInk.enabled ? 'Yes' : 'No',
        startHour: unbiddenInk.start_hour !== undefined ? `${String(unbiddenInk.start_hour).padStart(2, '0')}:00` : 'Unknown',
        endHour: unbiddenInk.end_hour !== undefined ? `${String(unbiddenInk.end_hour).padStart(2, '0')}:00` : 'Unknown',
        frequency: unbiddenInk.frequency_minutes !== undefined ? `${unbiddenInk.frequency_minutes} minutes` : 'Unknown',
        nextMessage: nextMessageText
      };
    },
    
    // Logging computed properties
    get loggingInfo() {
      const logging = this.diagnosticsData.features?.logging || {};
      return {
        level: logging.level_name || 'Unknown',
        serialLogging: logging.serial_enabled ? 'Enabled' : 'Disabled',
        webLogging: logging.betterstack_enabled ? 'Enabled' : 'Disabled',
        fileLogging: logging.file_enabled ? 'Enabled' : 'Disabled',
        mqttLogging: logging.mqtt_enabled ? 'Enabled' : 'Disabled'
      };
    },
    
    // Hardware buttons computed properties
    get hardwareButtonsInfo() {
      const buttonsData = this.diagnosticsData.features?.hardware_buttons || {};
      
      const baseInfo = {
        count: buttonsData.num_buttons || 'Unknown',
        debounce: `${buttonsData.debounce_ms || 'Unknown'} ms`,
        longPress: `${buttonsData.long_press_ms || 'Unknown'} ms`,
        activeLow: buttonsData.active_low !== undefined ? (buttonsData.active_low ? 'Yes' : 'No') : 'Unknown',
        minInterval: `${buttonsData.min_interval_ms || 'Unknown'} ms`,
        maxPerMinute: buttonsData.max_per_minute || 'Unknown'
      };
      
      const buttons = [];
      if (buttonsData.buttons && buttonsData.buttons.length > 0) {
        buttonsData.buttons.forEach((button, index) => {
          buttons.push({
            num: index + 1,
            gpio: button.gpio || 'Unknown',
            shortPress: button.short_endpoint || 'Not configured',
            longPress: button.long_endpoint || 'Not configured',
            shortMqtt: button.short_mqtt_topic || null,
            longMqtt: button.long_mqtt_topic || null
          });
        });
      }
      
      return { ...baseInfo, buttons };
    },
    
    // Web pages computed properties
    get webPages() {
      return this.diagnosticsData.endpoints?.web_pages || [];
    },
    
    // API endpoints computed properties
    get apiEndpoints() {
      const endpoints = this.diagnosticsData.endpoints?.api_endpoints || [];
      const grouped = {};
      
      endpoints.forEach(endpoint => {
        if (!grouped[endpoint.method]) {
          grouped[endpoint.method] = [];
        }
        grouped[endpoint.method].push(endpoint);
      });
      
      return grouped;
    },
    
    // Config file formatted
    get configFileFormatted() {
      if (!this.configData || Object.keys(this.configData).length === 0) {
        return 'Configuration not available';
      }
      
      const redacted = this.redactSecrets(this.configData);
      return JSON.stringify(redacted, null, 2);
    },
    
    // NVS data formatted
    get nvsDataFormatted() {
      if (!this.nvsData || Object.keys(this.nvsData).length === 0) {
        return 'NVS data not available';
      }
      
      const formattedData = {
        namespace: this.nvsData.namespace,
        timestamp: this.nvsData.timestamp,
        status: this.nvsData.status,
        summary: {
          totalKeys: this.nvsData.totalKeys || 0,
          validKeys: this.nvsData.validKeys || 0,
          correctedKeys: this.nvsData.correctedKeys || 0,
          invalidKeys: this.nvsData.invalidKeys || 0
        },
        keys: {}
      };
      
      if (this.nvsData.keys) {
        const sortedKeys = Object.keys(this.nvsData.keys).sort();
        sortedKeys.forEach(key => {
          const keyData = this.nvsData.keys[key];
          formattedData.keys[key] = {
            type: keyData.type,
            description: keyData.description,
            exists: keyData.exists,
            value: keyData.value,
            validation: keyData.validation,
            status: keyData.status
          };
          
          if (keyData.originalValue !== undefined) {
            formattedData.keys[key].originalValue = keyData.originalValue;
          }
          if (keyData.note) {
            formattedData.keys[key].note = keyData.note;
          }
          if (keyData.length !== undefined) {
            formattedData.keys[key].length = keyData.length;
          }
        });
      }
      
      return JSON.stringify(formattedData, null, 2);
    },
    
    // Copy functionality
    async copyGenericSection(sectionName) {
      try {
        let content = `=== ${sectionName} ===\n\n`;
        
        // Build content based on current section
        switch (this.currentSection) {
          case 'device-config-section':
            content += `Owner: ${this.deviceConfig.owner}\n`;
            content += `Timezone: ${this.deviceConfig.timezone}\n`;
            content += `mDNS Hostname: ${this.deviceConfig.mdnsHostname}\n`;
            content += `Max Message Chars: ${this.deviceConfig.maxMessageChars}\n`;
            break;
            
          case 'network-section':
            const network = this.networkInfo;
            content += `Wi-Fi Status: ${network.wifiStatus}\n`;
            content += `SSID: ${network.ssid}\n`;
            content += `IP Address: ${network.ipAddress}\n`;
            content += `Signal Strength: ${network.signalStrength}\n`;
            content += `MAC Address: ${network.macAddress}\n`;
            content += `Gateway: ${network.gateway}\n`;
            content += `DNS: ${network.dns}\n`;
            content += `Connect Timeout: ${network.connectTimeout}\n`;
            break;
            
          case 'mqtt-section':
            const mqtt = this.mqttInfo;
            content += `Enabled: ${mqtt.enabled}\n`;
            content += `Status: ${mqtt.status}\n`;
            content += `Broker: ${mqtt.broker}\n`;
            content += `Port: ${mqtt.port}\n`;
            content += `Topic: ${mqtt.topic}\n`;
            break;
            
          case 'microcontroller-section':
            const micro = this.microcontrollerInfo;
            const memory = this.memoryUsage;
            content += `Chip Model: ${micro.chipModel}\n`;
            content += `CPU Frequency: ${micro.cpuFrequency}\n`;
            content += `Flash Size: ${micro.flashSize}\n`;
            content += `Firmware Version: ${micro.firmwareVersion}\n`;
            content += `Uptime: ${micro.uptime}\n`;
            content += `Temperature: ${micro.temperature}\n`;
            content += `Flash Usage: ${memory.flashUsageText}\n`;
            content += `Heap Usage: ${memory.heapUsageText}\n`;
            break;
            
          // Add other sections as needed
          default:
            content += 'Section data not available for copying\n';
        }
        
        await this.copyToClipboard(content.trim());
      } catch (error) {
        console.error('Error copying section:', error);
      }
    },
    
    async copyConfigFile() {
      const content = `=== Runtime Configuration (NVS) ===\n\n${this.configFileFormatted}`;
      await this.copyToClipboard(content);
    },
    
    async copyNVSData() {
      const content = `=== NVS Storage Raw Data ===\n\n${this.nvsDataFormatted}`;
      await this.copyToClipboard(content);
    },
    
    async copyToClipboard(text) {
      try {
        if (navigator.clipboard && window.isSecureContext) {
          await navigator.clipboard.writeText(text);
        } else {
          // Fallback
          const textarea = document.createElement('textarea');
          textarea.value = text;
          textarea.style.position = 'fixed';
          textarea.style.opacity = '0';
          document.body.appendChild(textarea);
          textarea.select();
          document.execCommand('copy');
          document.body.removeChild(textarea);
        }
        
        // Visual feedback would be handled by button state in template
        console.log('Copied to clipboard');
      } catch (error) {
        console.error('Failed to copy:', error);
      }
    },
    
    // Quick actions
    async handleQuickAction(action) {
      try {
        const endpoint = `/api/${action}`;
        
        const response = await fetch(endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' }
        });

        if (!response.ok) {
          const errorData = await response.text();
          console.error(`Error generating content: ${errorData}`);
          return;
        }

        const contentResult = await response.json();
        
        if (!contentResult.content) {
          console.error('No content received from server');
          return;
        }

        // Print the content locally
        const printResponse = await fetch('/api/print-local', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ message: contentResult.content })
        });

        if (printResponse.ok) {
          console.log(`${action} sent to printer successfully!`);
        } else {
          const errorData = await printResponse.text();
          console.error(`Print error: ${errorData}`);
        }
      } catch (error) {
        console.error('Error sending quick action:', error);
      }
    },
    
    // Utility functions
    formatBytes(bytes) {
      if (bytes === 0) return '0 B';
      const k = 1024;
      const sizes = ['B', 'KB', 'MB', 'GB'];
      const i = Math.floor(Math.log(bytes) / Math.log(k));
      return parseFloat((bytes / Math.pow(k, i)).toFixed(0)) + ' ' + sizes[i];
    },
    
    formatUptime(seconds) {
      const days = Math.floor(seconds / 86400);
      const hours = Math.floor((seconds % 86400) / 3600);
      const minutes = Math.floor((seconds % 3600) / 60);
      if (days > 0) return `${days}d ${hours}h ${minutes}m`;
      if (hours > 0) return `${hours}h ${minutes}m`;
      return `${minutes}m`;
    },
    
    formatSignalStrength(dbm) {
      if (!dbm) return 'Unknown';
      
      let quality, color;
      
      if (dbm >= -30) {
        quality = 'Excellent';
        color = '#10b981';
      } else if (dbm >= -50) {
        quality = 'Very Good';
        color = '#10b981';
      } else if (dbm >= -60) {
        quality = 'Good';
        color = '#059669';
      } else if (dbm >= -70) {
        quality = 'Fair';
        color = '#f59e0b';
      } else if (dbm >= -80) {
        quality = 'Weak';
        color = '#ef4444';
      } else {
        quality = 'Very Weak';
        color = '#dc2626';
      }
      
      return `${dbm} dBm (${quality})`;
    },
    
    formatTemperature(tempC) {
      if (!tempC || isNaN(tempC)) return '-';
      
      let status, color;
      
      if (tempC < 35) {
        status = 'Cool';
        color = '#3b82f6';
      } else if (tempC < 50) {
        status = 'Normal';
        color = '#10b981';
      } else if (tempC < 65) {
        status = 'Warm';
        color = '#f59e0b';
      } else if (tempC < 80) {
        status = 'Hot';
        color = '#ef4444';
      } else {
        status = 'Critical';
        color = '#dc2626';
      }
      
      return `${tempC.toFixed(1)}°C (${status})`;
    },
    
    getProgressBarClass(percentage) {
      if (percentage > 90) return 'bg-red-500';
      if (percentage > 75) return 'bg-orange-600';
      return 'bg-orange-500';
    },
    
    redactSecrets(configData) {
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
    },
    
    // Navigation
    goBack() {
      window.location.href = '/';
    }
  };
  
  // Initialize data loading
  store.init();
  
  return store;
}

// Export for use in HTML
window.initializeDiagnosticsStore = initializeDiagnosticsStore;