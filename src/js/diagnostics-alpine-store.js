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
    currentSection: 'microcontroller-section',
    
    // Section definitions
    sections: [
      { id: 'microcontroller-section', name: 'Microcontroller', icon: '🎛️', color: 'orange' },
      { id: 'logging-section', name: 'Logging', icon: '📋', color: 'indigo' },
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
      
      console.log('🛠️ Diagnostics: Starting initialization...');
      await this.loadDiagnostics();
      console.log('🛠️ Diagnostics: Initialization complete, loading:', this.loading, 'error:', this.error);
    },
    
        // Load all diagnostics data with fallbacks for failed APIs
    async loadDiagnostics() {
      this.loading = true;
      this.error = null;
      console.log('🛠️ Diagnostics: Loading data from APIs...');
      
      try {
        console.log('🛠️ Diagnostics: Making parallel API calls...');
        // Load diagnostics, config, and NVS data in parallel with individual error handling
        const [diagnosticsResponse, configResponse, nvsResponse] = await Promise.all([
          fetch('/api/diagnostics').catch(() => null),
          fetch('/api/config').catch(() => null),
          fetch('/api/nvs-dump').catch(() => null)
        ]);
        
        console.log('🛠️ Diagnostics: API responses received:', {
          diagnostics: diagnosticsResponse?.ok || false,
          config: configResponse?.ok || false,
          nvs: nvsResponse?.ok || false
        });
        
        // Check if at least one API succeeded
        const anyApiSuccess = diagnosticsResponse?.ok || configResponse?.ok || nvsResponse?.ok;
        
        if (!anyApiSuccess) {
          // All APIs failed - this is an error state
          this.error = 'All diagnostic APIs are unavailable. Please check the system.';
          this.loading = false;
          return;
        }
        
        // Parse responses with fallbacks for failed APIs
        if (diagnosticsResponse?.ok) {
          this.diagnosticsData = await diagnosticsResponse.json();
        } else {
          console.warn('🛠️ Diagnostics: Diagnostics API failed, using fallback data');
          this.diagnosticsData = {}; // Empty object - computed properties provide fallbacks
        }
        
        if (configResponse?.ok) {
          this.configData = await configResponse.json();
        } else {
          console.warn('🛠️ Diagnostics: Config API failed, using fallback data');
          this.configData = {}; // Empty object - computed properties provide fallbacks  
        }
        
        if (nvsResponse?.ok) {
          this.nvsData = await nvsResponse.json();
        } else {
          console.warn('🛠️ Diagnostics: NVS API failed, using fallback data');
          this.nvsData = {}; // Empty object - computed properties provide fallbacks
        }
        
        console.log('🛠️ Diagnostics: Data loaded with fallbacks:', {
          diagnosticsKeys: Object.keys(this.diagnosticsData).length,
          configKeys: Object.keys(this.configData).length,
          nvsKeys: this.nvsData.keys ? Object.keys(this.nvsData.keys).length : 0
        });
        
        this.error = null;
        this.loading = false;
      } catch (error) {
        console.error('🛠️ Diagnostics: Unexpected error loading diagnostics:', error);
        // Unexpected error - show error state
        this.error = `Unexpected error loading diagnostics: ${error.message}`;
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
    
    // Microcontroller computed properties
    get microcontrollerInfo() {
      const microcontroller = this.diagnosticsData.microcontroller || {};
      
      return {
        chipModel: microcontroller.chip_model || 'Unknown',
        cpuFrequency: microcontroller.cpu_frequency_mhz ? `${microcontroller.cpu_frequency_mhz} MHz` : '-',
        flashSize: this.formatBytes(microcontroller.flash?.total_chip_size),
        firmwareVersion: microcontroller.sdk_version || '-',
        uptime: this.formatUptime(microcontroller.uptime_ms / 1000),
        temperature: this.formatTemperature(microcontroller.temperature)
      };
    },
    
    // Memory usage computed properties
    get memoryUsage() {
      const microcontroller = this.diagnosticsData.microcontroller || {};
      const flash = microcontroller.flash?.app_partition || {};
      const memory = microcontroller.memory || {};
      
      const flashUsed = flash.total ? ((flash.used || 0) / flash.total) * 100 : 0;
      const heapUsed = memory.total_heap ? ((memory.used_heap || 0) / memory.total_heap) * 100 : 0;
      
      return {
        flashUsageText: `${this.formatBytes(flash.used || 0)} / ${this.formatBytes(flash.total || 0)} (${flashUsed.toFixed(0)}%)`,
        heapUsageText: `${this.formatBytes(memory.used_heap || 0)} / ${this.formatBytes(memory.total_heap || 0)} (${heapUsed.toFixed(0)}%)`,
        flashUsagePercent: flashUsed,
        heapUsagePercent: heapUsed
      };
    },
    
    // Logging computed properties
    get loggingInfo() {
      const logging = this.diagnosticsData.logging || {};
      return {
        level: logging.level_name || 'Unknown',
        serialLogging: logging.serial_enabled ? 'Enabled' : 'Disabled',
        webLogging: logging.betterstack_enabled ? 'Enabled' : 'Disabled',
        fileLogging: logging.file_enabled ? 'Enabled' : 'Disabled',
        mqttLogging: logging.mqtt_enabled ? 'Enabled' : 'Disabled'
      };
    },
    
    // Web pages computed properties (renamed to sortedRoutes)
    get sortedRoutes() {
      const routes = this.diagnosticsData.pages_and_endpoints?.web_pages || [];
      
      // Separate HTML pages from other routes
      const htmlPages = [];
      const otherRoutes = [];
      
      routes.forEach(route => {
        if (route.path.endsWith('.html') || route.path === '/') {
          htmlPages.push({
            ...route,
            isHtmlPage: true,
            linkPath: route.path === '/' ? '/' : route.path
          });
        } else if (route.path === '(unmatched routes)') {
          otherRoutes.push({
            ...route,
            isUnmatched: true,
            linkPath: '/404',
            path: '*',
            description: '404 handler'
          });
        } else {
          otherRoutes.push({
            ...route,
            isHtmlPage: false
          });
        }
      });
      
      // Sort HTML pages alphabetically by path
      htmlPages.sort((a, b) => {
        // Put '/' first
        if (a.path === '/') return -1;
        if (b.path === '/') return 1;
        return a.path.localeCompare(b.path);
      });
      
      // Sort other routes alphabetically by path  
      otherRoutes.sort((a, b) => {
        // Put unmatched route (*) last
        if (a.isUnmatched) return 1;
        if (b.isUnmatched) return -1;
        return a.path.localeCompare(b.path);
      });
      
      // Combine: HTML pages first, then other routes
      return [...htmlPages, ...otherRoutes];
    },
    
    // API endpoints computed properties
    get apiEndpoints() {
      const endpoints = this.diagnosticsData.pages_and_endpoints?.api_endpoints || [];
      const grouped = {};
      
      endpoints.forEach(endpoint => {
        if (!grouped[endpoint.method]) {
          grouped[endpoint.method] = [];
        }
        grouped[endpoint.method].push(endpoint);
      });
      
      // Sort endpoints alphabetically within each method group
      Object.keys(grouped).forEach(method => {
        grouped[method].sort((a, b) => a.path.localeCompare(b.path));
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
  
  // Initialize data loading - Alpine.js will call init() automatically
  // No need to call it manually
  
  return store;
}

// Export for use in HTML
window.initializeDiagnosticsStore = initializeDiagnosticsStore;