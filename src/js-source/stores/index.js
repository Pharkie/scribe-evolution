/**
 * @file index.js
 * @brief Alpine.js reactive store for index page functionality (ES6 module)
 */

import {
  loadConfiguration,
  loadMemos,
  generateUserMessage,
  printLocalContent,
  printMQTT,
  executeQuickAction,
} from "../api/index.js";

/**
 * Create Alpine.js index store with data loading
 * @returns {Object} Alpine.js store object
 */
export function createIndexStore() {
  const store = {
    // Core state
    error: null,
    loaded: false, // Simple loading flag
    initialized: false, // Failsafe guard to prevent multiple inits
    data: {},

    // Form state
    message: "",
    selectedPrinter: "local-direct",
    submitting: false,
    buttonTextOverride: null,

    // Printer state
    printers: [],
    localPrinterName: "Unknown",

    // UI state
    overlayVisible: false,
    overlayPrinterData: null,
    overlayPrinterName: "",
    overlayPrinterType: "mqtt",

    // Settings stashed indicator
    settingsStashed: false,

    // Active quick action (only one can be active at a time)
    activeQuickAction: null,

    // Memo state
    memoModalVisible: false,
    memos: [],
    memosLoading: false,
    memosLoaded: false,
    printing: false,

    // Character limits - updated path for new structure
    get maxChars() {
      if (!this.loaded) return 384; // Default while loading

      if (!this.data.config?.device?.maxCharacters) {
        return 384; // Default fallback to prevent errors during initialization
      }
      return this.data.config.device.maxCharacters;
    },

    get charCount() {
      return this.message.length;
    },

    get charCountText() {
      const count = this.charCount;
      const max = this.maxChars;
      if (count > max) {
        const over = count - max;
        return `${count}/${max} characters (${over} over limit)`;
      }
      return `${count}/${max} characters`;
    },

    get charCountClass() {
      const count = this.charCount;
      const max = this.maxChars;
      const percentage = count / max;

      if (count > max) {
        // Over 100% - red and bold
        return "text-red-600 dark:text-red-400 font-semibold";
      } else if (percentage >= 0.9) {
        // 90-100% - yellow warning
        return "text-yellow-700 dark:text-yellow-300 font-medium";
      } else {
        // Under 90% - normal gray
        return "text-gray-500 dark:text-gray-400";
      }
    },

    get canSubmit() {
      return (
        this.message.trim().length > 0 &&
        this.charCount <= this.maxChars &&
        !this.isLoading
      );
    },

    get isConfigReady() {
      return (
        this.loaded && !this.error && this.data.config?.device?.maxCharacters
      );
    },

    // Initialize store
    async loadData() {
      // Duplicate initialization guard (failsafe)
      if (this.initialized) {
        console.log("📋 Index: Already initialized, skipping");
        return;
      }
      this.initialized = true;

      this.loaded = false;
      this.error = null;

      this.checkForSettingsSuccess();

      try {
        await this.loadConfig();

        // Always initialize local printer, conditionally initialize remote (MQTT) discovery
        this.initializeLocalPrinter();
        if (this.data.config?.mqtt?.enabled === true) {
          this.initializeRemotePrinterDiscovery();
        }

        this.setupEventListeners();
        this.loaded = true;
      } catch (error) {
        console.error("📋 Index: Config loading failed:", error);
        this.error = error.message;
      }
    },

    // Load configuration
    async loadConfig() {
      try {
        // Use ES6 imported API function instead of window.IndexAPI
        this.data.config = await loadConfiguration();

        if (this.data.config?.device?.printer_name === undefined) {
          throw new Error("Printer name configuration is missing from server");
        }
        if (this.data.config?.device?.maxCharacters === undefined) {
          throw new Error(
            "Maximum characters validation configuration is missing from server",
          );
        }
        if (this.data.config?.device === undefined) {
          throw new Error("Device configuration is missing from server");
        }

        this.localPrinterName = this.data.config.device.printer_name;

        // Load memos from separate API endpoint
        await this.loadMemosFromAPI();

        // Clear any previous error on success
        this.error = null;

        return this.data.config;
      } catch (error) {
        console.error("📋 Index: Failed to load config:", error);
        this.error = error.message;
        throw error; // Re-throw to ensure proper error handling
      }
    },

    // Initialize local printer (always available)
    initializeLocalPrinter() {
      this.updatePrinterList();
    },

    // Initialize remote printer (MQTT) discovery
    initializeRemotePrinterDiscovery() {
      this.setupSSEConnection();
    },

    // Update printer list from discovered printers
    updatePrinterList(discoveredPrinters = []) {
      this.printers = [
        {
          value: "local-direct",
          icon: "home",
          name: "Local direct",
          isLocal: true,
          selected: this.selectedPrinter === "local-direct",
        },
      ];

      // Add remote printers from discovered list
      discoveredPrinters.forEach((printer) => {
        const topic = `scribe/${printer.name}/print`;
        this.printers.push({
          value: topic,
          icon: "megaphone",
          name: printer.name,
          isLocal: false,
          data: printer,
          selected: this.selectedPrinter === topic,
        });
      });
    },

    // Setup event listeners
    setupEventListeners() {
      // Listen for printer updates from SSE
      document.addEventListener("printersUpdated", (event) => {
        this.updatePrinterList(event.detail.printers || []);
      });
    },

    // Select printer
    selectPrinter(value) {
      this.selectedPrinter = value;
      // Update selection status for existing printers instead of rebuilding the list
      this.printers.forEach((printer) => {
        printer.selected = printer.value === value;
      });
    },

    // Submit form
    async handleSubmit(event) {
      if (event) event.preventDefault();

      if (!this.canSubmit) return;

      this.submitting = true;

      try {
        const message = this.message.trim();

        // Step 1: Generate structured content with MESSAGE header using user-message endpoint
        const contentResult = await generateUserMessage(
          message,
          this.selectedPrinter,
        );

        if (!contentResult.header || !contentResult.body) {
          throw new Error("Failed to generate message content");
        }

        // Step 2: Print the formatted content
        if (this.selectedPrinter === "local-direct") {
          // For local printing, combine header + body into formatted string
          const formattedContent =
            contentResult.header + "\n\n" + contentResult.body;
          await printLocalContent(formattedContent);
        } else {
          // For MQTT printing, use structured data
          await printMQTT(
            contentResult.header,
            contentResult.body,
            this.selectedPrinter,
          );
        }

        // 🎊 Trigger confetti celebration for successful submission!
        this.triggerSubmitCelebration();

        // Clear form on success
        this.message = "";
      } catch (error) {
        console.error("Submit error:", error);
        this.error = `Error: ${error.message}`;
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

        // Get structured content from API
        const result = await executeQuickAction(action);

        if (!result.header) {
          this.error = "No content received from server";
          return;
        }

        // Format content based on target printer
        if (this.selectedPrinter === "local-direct") {
          // For local printing, combine header + body into formatted string
          // Handle empty body case (e.g., POKE action)
          const formattedContent = result.body
            ? result.header + "\n\n" + result.body
            : result.header;
          await printLocalContent(formattedContent);
        } else {
          // For MQTT printing, use structured data
          await printMQTT(result.header, result.body, this.selectedPrinter);
        }

        // 🎊 Trigger confetti celebration for successful quick action!
        this.triggerQuickActionCelebration(action);

        // Note: No success toast - button state change provides feedback
      } catch (error) {
        console.error("Error sending quick action:", error);
        this.error = `Network error: ${error.message}`;
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
      if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        if (this.canSubmit) {
          this.handleSubmit(event);
        }
      }
    },

    // Printer info overlay
    showLocalPrinterInfo() {
      if (!this.data.config?.device) {
        console.warn("Device configuration not loaded yet");
        return;
      }

      const deviceConfig = this.data.config.device;
      const localPrinterData = {
        name: deviceConfig.printer_name || deviceConfig.owner,
        ip_address: deviceConfig.ip_address,
        mdns: deviceConfig.mdns,
        status: "online",
        firmware_version: deviceConfig.firmware_version,
        timezone: deviceConfig.timezone,
        last_power_on: deviceConfig.boot_time,
      };

      this.showPrinterOverlay(localPrinterData, localPrinterData.name, "local");
    },

    showPrinterOverlay(printerData, printerName, printerType = "mqtt") {
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

      const topic =
        printerType === "mqtt"
          ? `scribe/${this.overlayPrinterName}/print`
          : null;
      const ipAddress = printerData.ip_address;
      const mdns = printerData.mdns;
      const firmwareVersion = printerData.firmware_version;
      const printerIcon =
        printerType === "local"
          ? window.getIcon("home", "w-6 h-6")
          : window.getIcon("megaphone", "w-6 h-6");

      // Format last power on time
      let lastPowerOnText = "Not available";
      if (printerData.last_power_on) {
        try {
          let powerOnTime;
          if (typeof printerData.last_power_on === "string") {
            powerOnTime = new Date(printerData.last_power_on);
          } else if (typeof printerData.last_power_on === "number") {
            const timestamp =
              printerData.last_power_on < 10000000000
                ? printerData.last_power_on * 1000
                : printerData.last_power_on;
            powerOnTime = new Date(timestamp);
          } else {
            powerOnTime = new Date(printerData.last_power_on);
          }

          const now = new Date();
          const diffMs = now.getTime() - powerOnTime.getTime();
          const lastPowerOnRelative = this.formatTimeDifference(diffMs);
          const lastPowerOnAbsolute = powerOnTime.toLocaleString(undefined, {
            year: "numeric",
            month: "short",
            day: "numeric",
            hour: "2-digit",
            minute: "2-digit",
            second: "2-digit",
            hour12: false,
          });

          lastPowerOnText = `${lastPowerOnRelative}${lastPowerOnAbsolute ? ` (${lastPowerOnAbsolute})` : ""}`;
        } catch (e) {
          lastPowerOnText = "Invalid date";
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
        timezone,
      };
    },

    // Copy topic to clipboard
    async copyTopic(topic) {
      try {
        if (navigator.clipboard && window.isSecureContext) {
          await navigator.clipboard.writeText(topic);
        } else {
          // Fallback
          const textarea = document.createElement("textarea");
          textarea.value = topic;
          textarea.style.position = "fixed";
          textarea.style.left = "-999999px";
          textarea.style.top = "-999999px";
          document.body.appendChild(textarea);
          textarea.focus();
          textarea.select();
          document.execCommand("copy");
          document.body.removeChild(textarea);
        }
        // Alpine will handle the visual feedback via $dispatch
      } catch (error) {
        console.error("Failed to copy:", error);
        this.error = "Failed to copy topic";
      }
    },

    // Format time difference
    formatTimeDifference(diffMs) {
      const diffSeconds = Math.floor(diffMs / 1000);
      const diffMinutes = Math.floor(diffSeconds / 60);
      const diffHours = Math.floor(diffMinutes / 60);
      const diffDays = Math.floor(diffHours / 24);

      if (diffDays > 0) {
        return `${diffDays} day${diffDays > 1 ? "s" : ""} ago`;
      } else if (diffHours > 0) {
        return `about ${diffHours} hour${diffHours > 1 ? "s" : ""} ago`;
      } else if (diffMinutes >= 2) {
        return `${diffMinutes} mins ago`;
      } else if (diffMinutes === 1) {
        return "A minute ago";
      } else if (diffSeconds > 30) {
        return "30 seconds ago";
      } else {
        return "Just now";
      }
    },

    // Toast management removed in favor of inline error state

    // Check for settings success
    checkForSettingsSuccess() {
      const urlParams = new URLSearchParams(window.location.search);
      if (urlParams.get("settings") === "stashed") {
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
      else if (urlParams.get("settings_saved") === "true") {
        // Legacy param: map to settingsStashed indicator for consistency
        this.settingsStashed = true;
        setTimeout(() => {
          this.settingsStashed = false;
        }, 3000);

        // Clean up URL
        const cleanUrl = window.location.pathname;
        window.history.replaceState({}, document.title, cleanUrl);
      }
    },

    // Confetti Celebration Methods
    triggerQuickActionCelebration(action) {
      if (typeof confetti !== "undefined") {
        const buttonElement = document.querySelector(
          `[data-action="${action}"]`,
        );
        const buttonRect = buttonElement?.getBoundingClientRect();

        // Different effects for different actions
        switch (action) {
          case "riddle":
            // 🧩 Puzzle pieces effect for riddles
            confetti({
              particleCount: 100,
              spread: 70,
              origin: buttonRect
                ? {
                    x:
                      (buttonRect.left + buttonRect.width / 2) /
                      window.innerWidth,
                    y:
                      (buttonRect.top + buttonRect.height / 2) /
                      window.innerHeight,
                  }
                : { y: 0.6 },
              colors: ["#f59e0b", "#eab308", "#facc15", "#fde047"], // Yellow tones
              shapes: ["square"],
            });
            break;

          case "joke":
            // 😄 Happy burst for jokes
            confetti({
              particleCount: 150,
              spread: 90,
              origin: buttonRect
                ? {
                    x:
                      (buttonRect.left + buttonRect.width / 2) /
                      window.innerWidth,
                    y:
                      (buttonRect.top + buttonRect.height / 2) /
                      window.innerHeight,
                  }
                : { y: 0.6 },
              colors: ["#10b981", "#34d399", "#6ee7b7", "#a7f3d0"], // Emerald tones
              scalar: 1.2,
            });
            break;

          case "quote":
            // ✨ Elegant sparkles for quotes
            confetti({
              particleCount: 80,
              spread: 45,
              origin: buttonRect
                ? {
                    x:
                      (buttonRect.left + buttonRect.width / 2) /
                      window.innerWidth,
                    y:
                      (buttonRect.top + buttonRect.height / 2) /
                      window.innerHeight,
                  }
                : { y: 0.6 },
              colors: ["#8b5cf6", "#a78bfa", "#c4b5fd", "#e0e7ff"], // Purple tones
              scalar: 0.8,
              shapes: ["star"],
            });
            break;

          case "quiz":
            // 🎯 Target burst for quiz
            confetti({
              particleCount: 120,
              spread: 360,
              origin: buttonRect
                ? {
                    x:
                      (buttonRect.left + buttonRect.width / 2) /
                      window.innerWidth,
                    y:
                      (buttonRect.top + buttonRect.height / 2) /
                      window.innerHeight,
                  }
                : { y: 0.6 },
              colors: ["#06b6d4", "#67e8f9", "#a5f3fc", "#cffafe"], // Cyan tones
              startVelocity: 45,
              decay: 0.85,
            });
            break;

          case "news":
            // 📰 Newspaper effect - gray and white squares/rectangles to match gray button
            confetti({
              particleCount: 120,
              spread: 80,
              origin: buttonRect
                ? {
                    x:
                      (buttonRect.left + buttonRect.width / 2) /
                      window.innerWidth,
                    y:
                      (buttonRect.top + buttonRect.height / 2) /
                      window.innerHeight,
                  }
                : { y: 0.6 },
              colors: ["#6b7280", "#9ca3af", "#d1d5db", "#f3f4f6"], // Gray tones to match gray button
              shapes: ["square"],
              scalar: 1.1,
              gravity: 0.9,
              drift: 0.05,
            });
            break;

          case "memo":
            // 📝 Pink sparkles for memos
            confetti({
              particleCount: 100,
              spread: 60,
              origin: buttonRect
                ? {
                    x:
                      (buttonRect.left + buttonRect.width / 2) /
                      window.innerWidth,
                    y:
                      (buttonRect.top + buttonRect.height / 2) /
                      window.innerHeight,
                  }
                : { y: 0.6 },
              colors: ["#ec4899", "#f472b6", "#f9a8d4", "#fce7f3"], // Pink tones to match pink button
              scalar: 0.9,
              startVelocity: 30,
            });
            break;

          default:
            // Default celebration
            confetti({
              particleCount: 100,
              spread: 70,
              origin: buttonRect
                ? {
                    x:
                      (buttonRect.left + buttonRect.width / 2) /
                      window.innerWidth,
                    y:
                      (buttonRect.top + buttonRect.height / 2) /
                      window.innerHeight,
                  }
                : { y: 0.6 },
            });
        }
      }
    },

    triggerSubmitCelebration() {
      if (typeof confetti !== "undefined") {
        const submitButton = document.querySelector("#main-submit-btn");
        const buttonRect = submitButton?.getBoundingClientRect();

        // 🖨️ Printer celebration with single blue burst
        const origin = buttonRect
          ? {
              x: (buttonRect.left + buttonRect.width / 2) / window.innerWidth,
              y: (buttonRect.top + buttonRect.height / 2) / window.innerHeight,
            }
          : { y: 0.6 };

        // Single blue burst celebration
        confetti({
          particleCount: 200,
          spread: 100,
          origin,
          colors: ["#3b82f6", "#60a5fa", "#93c5fd", "#dbeafe"], // Blue tones only
          scalar: 1.5,
        });
      }
    },

    // Navigation
    goToSettings() {
      window.location.href = "/settings/"; // New modular settings overview page
    },

    // === Memo Functions ===

    async loadMemosFromAPI() {
      // Don't reload if already loaded
      if (this.memosLoaded) return;

      try {
        const memosData = await loadMemos();

        // Convert API format to modal format
        this.memos = [
          { id: 1, content: memosData.memo1 || "" },
          { id: 2, content: memosData.memo2 || "" },
          { id: 3, content: memosData.memo3 || "" },
          { id: 4, content: memosData.memo4 || "" },
        ];

        this.memosLoaded = true;
      } catch (error) {
        console.error("📝 Failed to load memos:", error);
        // Fallback to empty memos
        this.memos = [
          { id: 1, content: "" },
          { id: 2, content: "" },
          { id: 3, content: "" },
          { id: 4, content: "" },
        ];
        this.memosLoaded = true;
      }
    },

    async showMemoModal() {
      this.memoModalVisible = true;

      // Ensure memos are loaded
      if (!this.memosLoaded) {
        await this.loadMemosFromAPI();
      }
    },

    closeMemoModal() {
      this.memoModalVisible = false;
    },

    async printMemo(memoId) {
      if (this.printing) return;

      try {
        this.printing = true;

        // Step 1: Get processed memo content (clean GET request)
        const response = await fetch(`/api/memo/${memoId}`);

        if (!response.ok) {
          throw new Error(`Failed to get memo: ${response.status}`);
        }

        const memoData = await response.json();
        if (!memoData.header || !memoData.body) {
          throw new Error("No memo content received");
        }

        // Step 2: Print using the same method as other buttons
        if (this.selectedPrinter === "local-direct") {
          // Format as string for local printing
          const formattedContent = memoData.header + "\n\n" + memoData.body;
          await printLocalContent(formattedContent);
        } else {
          // Send structured data for MQTT printing
          await printMQTT(memoData.header, memoData.body, this.selectedPrinter);
        }

        // HTTP 200 status indicates success - no need to check response body
        // Set active action to show "Scribed" on memo button
        this.activeQuickAction = "memo";
        this.closeMemoModal();

        // Pink sparkle confetti celebration (keep the confetti!)
        if (window.confetti) {
          confetti({
            colors: ["#ec4899", "#f472b6", "#f9a8d4", "#fce7f3"], // Pink tones to match pink button
            startVelocity: 30,
            spread: 360,
            ticks: 60,
            zIndex: 0,
          });
        }

        // Reset active action after 2 seconds like other quick actions
        setTimeout(() => {
          this.activeQuickAction = null;
        }, 2000);
      } catch (error) {
        console.error("Failed to print memo:", error);
        this.error = `Failed to print memo: ${error.message}`;
      } finally {
        this.printing = false;
      }
    },

    // Setup SSE connection for remote printer (MQTT) discovery
    setupSSEConnection() {
      let eventSource = null;

      const connectSSE = () => {
        // Close existing connection if any
        if (eventSource) {
          eventSource.close();
        }

        // Create new SSE connection
        eventSource = new EventSource("/mqtt-printers");

        // Handle remote printer (MQTT) updates
        eventSource.addEventListener("printer-update", (event) => {
          try {
            const data = JSON.parse(event.data);
            this.updatePrintersFromData(data);
          } catch (error) {
            console.error("Error parsing remote printer (MQTT) update:", error);
          }
        });

        // Handle system status updates (pipe into inline error state)
        eventSource.addEventListener("system-status", (event) => {
          try {
            const data = JSON.parse(event.data);
            const status = data.status;
            const message = data.message || "";
            if (status === "connected") {
              // Clear error on successful connection
              this.error = null;
            } else if (status === "error" || status === "reconnecting") {
              // Show status message as inline error
              this.error =
                message ||
                (status === "reconnecting"
                  ? "Reconnecting to printer service"
                  : "System error");
            }
          } catch (error) {
            console.error("Error parsing system status:", error);
          }
        });

        // Handle connection errors
        eventSource.onerror = (error) => {
          console.error(
            "SSE connection error for remote printer (MQTT) discovery:",
            error,
          );
          // Surface as inline error
          this.error = "Connection error: remote printer discovery";
          // Attempt to reconnect after 5 seconds
          setTimeout(() => {
            connectSSE();
          }, 5000);
        };

        // Handle successful connection
        eventSource.onopen = (event) => {
          // Connection established
        };
      };

      // Start initial connection
      connectSSE();

      // Clean up on page unload
      window.addEventListener("beforeunload", () => {
        if (eventSource) {
          eventSource.close();
        }
      });
    },

    updatePrintersFromData(data) {
      if (data && data.discovered_printers) {
        // Update the store's printers array directly
        this.printers = data.discovered_printers;

        // Also dispatch custom event for backward compatibility if needed
        const event = new CustomEvent("printersUpdated", {
          detail: {
            printers: data.discovered_printers,
          },
        });
        document.dispatchEvent(event);
      }
    },

    // Removed SSE notification popups; using inline error state instead
  };

  return store;
}
