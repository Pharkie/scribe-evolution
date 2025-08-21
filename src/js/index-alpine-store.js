/**
 * @file index-alpine-store.js
 * @brief Alpine.js reactive store for index page functionality
 */

// Initialize Alpine.js index store with data loading
function initializeIndexStore() {
  const store = {
    // Core state
    config: {},
    loading: true,
    error: null,
    initialized: false, // Flag to prevent duplicate initialization
    
    // Form state
    message: '',
    selectedPrinter: 'local-direct',
    submitting: false,
    buttonTextOverride: null,
    
    // Printer state
    printers: [],
    localPrinterName: 'Unknown',
    
    // UI state
    overlayVisible: false,
    overlayPrinterData: null,
    overlayPrinterName: '',
    overlayPrinterType: 'mqtt',
    
    // Settings stashed indicator
    settingsStashed: false,
    
    // Toast state
    toasts: [],
    
    // Active quick action (only one can be active at a time)
    activeQuickAction: null,
    
    // Character limits - updated path for new structure
    get maxChars() {
      if (!this.config?.device?.maxCharacters) {
        throw new Error('Maximum characters configuration is missing from server');
      }
      return this.config.device.maxCharacters;
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
      return this.isConfigReady && this.message.trim() && !this.submitting && this.charCount <= this.maxChars;
    },
    
    get isConfigReady() {
      return !this.loading && !this.error && this.config && this.config.device && this.config.device.maxCharacters;
    },
    
    // Initialize store
    async init() {
      // Prevent duplicate initialization
      if (this.initialized) {
        console.log('📋 Index: Already initialized, skipping');
        return;
      }
      this.initialized = true;
      
      console.log('📋 Index: Starting initialization...');
      this.checkForSettingsSuccess();
      
      try {
        await this.loadConfig();
      } catch (error) {
        console.error('📋 Index: Config loading failed:', error);
        // Set error state - Alpine.js standard pattern
        this.error = error.message;
        this.loading = false;
      }
      
      this.initializePrinterDiscovery();
      this.setupEventListeners();
      console.log('📋 Index: Initialization complete');
    },
    
    // Load configuration
    async loadConfig() {
      try {
        console.log('📋 Index: Loading configuration from API...');
        
        // Use API layer instead of direct fetch
        this.config = await window.IndexAPI.loadConfiguration();
        
        console.log('📋 Index: Raw config received:', this.config);
        console.log('📋 Index: Config keys:', Object.keys(this.config));
        
        if (this.config?.device?.printer_name === undefined) {
          throw new Error('Printer name configuration is missing from server');
        }
        if (this.config?.device?.maxCharacters === undefined) {
          throw new Error('Maximum characters validation configuration is missing from server');
        }
        if (this.config?.device === undefined) {
          throw new Error('Device configuration is missing from server');
        }
        
        this.localPrinterName = this.config.device.printer_name;
        console.log('📋 Index: Config loaded successfully, printer name:', this.localPrinterName);
        
        // Clear any previous error and set loading to false on success
        this.error = null;
        this.loading = false;
        
        return this.config;
      } catch (error) {
        console.error('📋 Index: Failed to load config:', error);
        this.error = error.message;
        this.loading = false;
        throw error; // Re-throw to ensure proper error handling
      }
    },
    
    // Initialize printer discovery
    initializePrinterDiscovery() {
      // Start SSE for printer discovery
      if (typeof window.initializePrinterDiscovery === 'function') {
        window.initializePrinterDiscovery();
      }
      
      this.updatePrinterList();
    },
    
    // Update printer list from discovered printers
    updatePrinterList(discoveredPrinters = []) {
      this.printers = [
        {
          value: 'local-direct',
          icon: '🏠',
          name: 'Local direct',
          isLocal: true,
          selected: this.selectedPrinter === 'local-direct'
        }
      ];
      
      // Add remote printers from discovered list
      discoveredPrinters.forEach(printer => {
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
    },
    
    // Setup event listeners
    setupEventListeners() {
      // Listen for printer updates from SSE
      document.addEventListener('printersUpdated', (event) => {
        console.log('🔄 Printers updated, refreshing index page printer selection');
        this.updatePrinterList(event.detail.printers || []);
      });
    },
    
    // Select printer
    selectPrinter(value) {
      this.selectedPrinter = value;
      // Update selection status for existing printers instead of rebuilding the list
      this.printers.forEach(printer => {
        printer.selected = printer.value === value;
      });
    },
    
    // Submit form
    async handleSubmit(event) {
      if (event) event.preventDefault();
      
      if (!this.canSubmit) return;
      
      this.submitting = true;
      
      try {
        let formData;
        let endpoint;
        
        // Build payload based on printer target
        if (this.selectedPrinter === 'local-direct') {
          formData = {
            message: this.message.trim()
          };
          endpoint = '/api/print-local';
        } else {
          // For MQTT printing, use "topic" instead of "printer-target"
          formData = {
            message: this.message.trim(),
            topic: this.selectedPrinter
          };
          endpoint = '/api/print-mqtt';
        }
        
        const message = this.message.trim();
        
        // Use API layer based on printer target
        if (this.selectedPrinter === 'local-direct') {
          await window.IndexAPI.printLocalContent(message);
        } else {
          await window.IndexAPI.printMQTTContent(message, this.selectedPrinter);
        }
        
        // Clear form on success
        this.message = '';
        
      } catch (error) {
        console.error('Submit error:', error);
        this.showToast(`Error: ${error.message}`, 'error');
      } finally {
        this.submitting = false;
      }
    },
    
    // Quick actions
    async sendQuickAction(action) {
      // Prevent double-clicking while any action is in progress
      if (this.activeQuickAction) {
        return;
      }
      
      try {
        // Set active action - Alpine.js will reactively update the UI
        this.activeQuickAction = action;
        
        // Use API layer for executing quick action
        const contentResult = await window.IndexAPI.executeQuickAction(action);
        
        if (!contentResult.content) {
          this.showToast('No content received from server', 'error');
          return;
        }

        // Use API layer for printing content
        if (this.selectedPrinter === 'local-direct') {
          await window.IndexAPI.printLocalContent(contentResult.content);
        } else {
          await window.IndexAPI.printMQTTContent(contentResult.content, this.selectedPrinter);
        }
        // Note: No success toast - button state change provides feedback
        
      } catch (error) {
        console.error('Error sending quick action:', error);
        this.showToast(`Network error: ${error.message}`, 'error');
      } finally {
        // Reset active action after 2 seconds - Alpine.js will reactively update UI
        setTimeout(() => {
          this.activeQuickAction = null;
        }, 2000);
      }
    },
    
    // Handle textarea keydown
    handleTextareaKeydown(event) {
      // Enter to submit (unless Shift is held for newline)
      if (event.key === 'Enter' && !event.shiftKey) {
        event.preventDefault();
        if (this.canSubmit) {
          this.handleSubmit(event);
        }
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
      this.overlayVisible = true;
    },
    
    closePrinterOverlay() {
      this.overlayVisible = false;
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
        // Alpine will handle the visual feedback via $dispatch
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
      if (urlParams.get('settings') === 'stashed') {
        // Show "Stashed" indicator on settings button for 3 seconds with orange fade animation
        this.settingsStashed = true;
        setTimeout(() => {
          this.settingsStashed = false;
        }, 3000);
        
        // Clean up URL
        const cleanUrl = window.location.pathname;
        window.history.replaceState({}, document.title, cleanUrl);
      }
      // Legacy support for old parameter
      else if (urlParams.get('settings_saved') === 'true') {
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
  
  // Initialize data loading - Alpine.js will call init() automatically
  // No need to call it manually
  
  return store;
}

// Export for use in HTML
window.initializeIndexStore = initializeIndexStore;