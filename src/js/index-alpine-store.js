/**
 * @file index-alpine-store.js
 * @brief Alpine.js reactive store for index page functionality
 */

// Alpine.js Store for Index Page
window.IndexStore = function() {
  return {
    // Core state
    config: {},
    loading: true,
    error: null,
    initialized: false, // Flag to prevent duplicate initialization
    
    // Form state
    message: '',
    selectedPrinter: 'local-direct',
    submitting: false,
    
    // Printer state
    printers: [],
    localPrinterName: 'Unknown',
    
    // UI state
    showPrinterOverlay: false,
    overlayPrinterData: null,
    overlayPrinterName: '',
    overlayPrinterType: 'mqtt',
    
    // Toast state
    toasts: [],
    
    // Character limits
    get maxChars() {
      if (this.config?.validation?.maxCharacters === undefined) {
        throw new Error('Maximum characters configuration is missing from server');
      }
      return this.config.validation.maxCharacters;
    },
    
    get charCount() {
      return this.message.length;
    },
    
    get charCountText() {
      return `${this.charCount}/${this.maxChars} characters`;
    },
    
    get charCountClass() {
      if (this.charCount > this.maxChars) {
        return 'text-red-600 dark:text-red-400';
      } else if (this.charCount >= this.maxChars * 0.8) {
        return 'text-yellow-700 dark:text-yellow-300';
      } else {
        return 'text-gray-500 dark:text-gray-400';
      }
    },
    
    get canSubmit() {
      return this.message.trim() && !this.submitting && this.charCount <= this.maxChars;
    },
    
    // Initialize store
    async init() {
      // Prevent duplicate initialization
      if (this.initialized) {
        console.log('📋 Index: Already initialized, skipping');
        return;
      }
      this.initialized = true;
      
      this.checkForSettingsSuccess();
      await this.loadConfig();
      this.initializePrinterDiscovery();
      this.setupEventListeners();
    },
    
    // Load configuration
    async loadConfig() {
      try {
        console.log('📋 Index: Loading configuration...');
        const response = await fetch('/api/config');
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        this.config = await response.json();
        
        if (this.config?.printer?.name === undefined) {
          throw new Error('Printer name configuration is missing from server');
        }
        this.localPrinterName = this.config.printer.name;
        console.log('📋 Index: Config loaded successfully');
        this.loading = false;
        
        return this.config;
      } catch (error) {
        console.error('📋 Index: Failed to load config:', error);
        this.error = error.message;
        this.loading = false;
        // Continue with defaults - page should still work
        return {};
      }
    },
    
    // Initialize printer discovery
    initializePrinterDiscovery() {
      // Initialize global PRINTERS array if not exists
      if (typeof window.PRINTERS === 'undefined') {
        window.PRINTERS = [];
      }
      
      // Start SSE for printer discovery
      if (typeof window.startPrinterDiscovery === 'function') {
        window.startPrinterDiscovery();
      }
      
      this.updatePrinterList();
    },
    
    // Update printer list from global PRINTERS
    updatePrinterList() {
      this.printers = [
        {
          value: 'local-direct',
          icon: '🏠',
          name: 'Local direct',
          isLocal: true,
          selected: this.selectedPrinter === 'local-direct'
        }
      ];
      
      // Add remote printers
      if (typeof window.PRINTERS !== 'undefined') {
        window.PRINTERS.forEach(printer => {
          const topic = `scribe/${printer.name}/print`;
          this.printers.push({
            value: topic,
            icon: '📡',
            name: printer.name,
            isLocal: false,
            data: printer,
            selected: this.selectedPrinter === topic
          });
        });
      }
    },
    
    // Setup event listeners
    setupEventListeners() {
      // Listen for printer updates from SSE
      document.addEventListener('printersUpdated', () => {
        console.log('🔄 Printers updated, refreshing index page printer selection');
        this.updatePrinterList();
      });
    },
    
    // Select printer
    selectPrinter(value) {
      this.selectedPrinter = value;
      this.updatePrinterList();
    },
    
    // Submit form
    async handleSubmit(event) {
      if (event) event.preventDefault();
      
      if (!this.canSubmit) return;
      
      this.submitting = true;
      
      try {
        const formData = {
          message: this.message.trim(),
          'printer-target': this.selectedPrinter
        };
        
        // Determine endpoint based on printer target
        const endpoint = this.selectedPrinter === 'local-direct' ? '/api/print-local' : '/api/print-mqtt';
        
        const response = await fetch(endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(formData)
        });
        
        if (response.ok) {
          const result = await response.json();
          this.showToast(result.message || 'Message sent successfully!', 'success');
          this.message = ''; // Clear form
        } else {
          const errorData = await response.text();
          this.showToast(`Error: ${errorData}`, 'error');
        }
      } catch (error) {
        console.error('Submit error:', error);
        this.showToast(`Network error: ${error.message}`, 'error');
      } finally {
        this.submitting = false;
      }
    },
    
    // Quick actions
    async sendQuickAction(action) {
      try {
        const endpoint = `/api/${action}`;
        
        const response = await fetch(endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' }
        });

        if (!response.ok) {
          const errorData = await response.text();
          this.showToast(`Error generating content: ${errorData}`, 'error');
          return;
        }

        const contentResult = await response.json();
        
        if (!contentResult.content) {
          this.showToast('No content received from server', 'error');
          return;
        }

        // Print the content locally
        const printResponse = await fetch('/api/print-local', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ message: contentResult.content })
        });

        if (printResponse.ok) {
          this.showToast(`${action} sent to printer successfully!`, 'success');
        } else {
          const errorData = await printResponse.text();
          this.showToast(`Print error: ${errorData}`, 'error');
        }
      } catch (error) {
        console.error('Error sending quick action:', error);
        this.showToast(`Network error: ${error.message}`, 'error');
      }
    },
    
    // Handle textarea keydown
    handleTextareaKeydown(event) {
      // Ctrl+Enter or Cmd+Enter to submit
      if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
        event.preventDefault();
        this.handleSubmit();
      }
    },
    
    // Printer info overlay
    showLocalPrinterInfo() {
      if (!this.config?.device) {
        throw new Error('Device configuration is missing from server');
      }
      
      const deviceConfig = this.config.device;
      const localPrinterData = {
        name: deviceConfig.printer_name || deviceConfig.owner,
        ip_address: deviceConfig.ip_address,
        mdns: deviceConfig.mdns,
        status: 'online',
        firmware_version: deviceConfig.firmware_version,
        timezone: deviceConfig.timezone,
        last_power_on: deviceConfig.boot_time
      };
      
      this.showPrinterOverlay(localPrinterData, localPrinterData.name, 'local');
    },
    
    showPrinterOverlay(printerData, printerName, printerType = 'mqtt') {
      this.overlayPrinterData = printerData;
      this.overlayPrinterName = printerName;
      this.overlayPrinterType = printerType;
      this.showPrinterOverlay = true;
    },
    
    closePrinterOverlay() {
      this.showPrinterOverlay = false;
      this.overlayPrinterData = null;
    },
    
    // Get formatted printer overlay data
    get overlayData() {
      if (!this.overlayPrinterData) return null;
      
      const printerData = this.overlayPrinterData;
      const printerType = this.overlayPrinterType;
      
      const topic = printerType === 'mqtt' ? `scribe/${this.overlayPrinterName}/print` : null;
      const ipAddress = printerData.ip_address;
      const mdns = printerData.mdns; 
      const firmwareVersion = printerData.firmware_version;
      const printerIcon = printerType === 'local' ? '🏠' : '📡';
      
      // Format last power on time
      let lastPowerOnText = 'Not available';
      if (printerData.last_power_on) {
        try {
          let powerOnTime;
          if (typeof printerData.last_power_on === 'string') {
            powerOnTime = new Date(printerData.last_power_on);
          } else if (typeof printerData.last_power_on === 'number') {
            const timestamp = printerData.last_power_on < 10000000000 ? 
              printerData.last_power_on * 1000 : printerData.last_power_on;
            powerOnTime = new Date(timestamp);
          } else {
            powerOnTime = new Date(printerData.last_power_on);
          }
          
          const now = new Date();
          const diffMs = now.getTime() - powerOnTime.getTime();
          const lastPowerOnRelative = this.formatTimeDifference(diffMs);
          const lastPowerOnAbsolute = powerOnTime.toLocaleString(undefined, {
            year: 'numeric',
            month: 'short',
            day: 'numeric',
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit',
            hour12: false
          });
          
          lastPowerOnText = `${lastPowerOnRelative}${lastPowerOnAbsolute ? ` (${lastPowerOnAbsolute})` : ''}`;
        } catch (e) {
          lastPowerOnText = 'Invalid date';
        }
      }
      
      const timezone = printerData.timezone;
      
      return {
        topic,
        ipAddress,
        mdns,
        firmwareVersion,
        printerIcon,
        lastPowerOnText,
        timezone
      };
    },
    
    // Copy topic to clipboard
    async copyTopic(topic) {
      try {
        if (navigator.clipboard && window.isSecureContext) {
          await navigator.clipboard.writeText(topic);
        } else {
          // Fallback
          const textarea = document.createElement('textarea');
          textarea.value = topic;
          textarea.style.position = 'fixed';
          textarea.style.left = '-999999px';
          textarea.style.top = '-999999px';
          document.body.appendChild(textarea);
          textarea.focus();
          textarea.select();
          document.execCommand('copy');
          document.body.removeChild(textarea);
        }
        this.showToast('Topic copied to clipboard', 'success');
      } catch (error) {
        console.error('Failed to copy:', error);
        this.showToast('Failed to copy topic', 'error');
      }
    },
    
    // Format time difference
    formatTimeDifference(diffMs) {
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
    },
    
    // Toast management
    showToast(message, type = 'info') {
      const id = Date.now();
      const toast = { id, message, type };
      this.toasts.push(toast);
      
      setTimeout(() => {
        this.removeToast(id);
      }, 4000);
    },
    
    removeToast(id) {
      this.toasts = this.toasts.filter(toast => toast.id !== id);
    },
    
    // Check for settings success
    checkForSettingsSuccess() {
      const urlParams = new URLSearchParams(window.location.search);
      if (urlParams.get('settings_saved') === 'true') {
        this.showToast('💾 Settings saved', 'success');
        
        // Clean up URL
        const cleanUrl = window.location.pathname;
        window.history.replaceState({}, document.title, cleanUrl);
      }
    },
    
    // Navigation
    goToSettings() {
      window.location.href = '/settings.html';
    }
  };
};