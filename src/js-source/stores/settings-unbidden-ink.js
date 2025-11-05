/**
 * @file settings-unbidden-ink.js
 * @brief Alpine.js store factory for Unbidden Ink settings page
 * @description Handles AI content scheduling, dual-handle slider, frequency settings
 * Converted from legacy JavaScript to ES6 modules following established patterns
 */

import { loadConfiguration, saveConfiguration } from "../api/settings.js";

export function createSettingsUnbiddenInkStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false, // Simple loading flag (starts false)
    saving: false,
    error: null,
    initialized: false,

    // Configuration data (empty object populated on load)
    config: {},

    originalValues: {},
    apiTokenModified: false,
    // Track connection test state for button label logic
    chatgptTested: false,
    chatgptTestSuccess: false,

    // Validation state
    validation: {
      errors: {},
    },

    // ================== API OPERATIONS ==================
    async loadConfiguration() {
      // Duplicate initialization guard (failsafe)
      if (this.initialized) {
        return;
      }
      this.initialized = true;

      this.loaded = false;
      this.error = null;

      try {
        const config = await loadConfiguration();

        // ✅ CRITICAL: Direct assignment to config object
        this.config.unbiddenInk = {
          enabled: config.unbiddenInk?.enabled || false,
          startHour: config.unbiddenInk?.startHour || 8,
          endHour: config.unbiddenInk?.endHour || 22,
          frequencyMinutes: config.unbiddenInk?.frequencyMinutes || 120,
          prompt: config.unbiddenInk?.prompt || "",
          chatgptApiToken: config.unbiddenInk?.chatgptApiToken || "",
          promptPresets: config.unbiddenInk?.promptPresets || {},
          status: config.unbiddenInk?.status || "Disabled",
          nextScheduled: config.unbiddenInk?.nextScheduled || "-",
        };

        // Store original values for change tracking
        this.originalValues = {
          enabled: this.config.unbiddenInk.enabled,
          startHour: this.config.unbiddenInk.startHour,
          endHour: this.config.unbiddenInk.endHour,
          frequencyMinutes: this.config.unbiddenInk.frequencyMinutes,
          prompt: this.config.unbiddenInk.prompt,
          chatgptApiToken: this.config.unbiddenInk.chatgptApiToken,
        };

        // Reset token/test state on load
        this.apiTokenModified = false;
        this.chatgptTesting = false;
        this.chatgptTested = false;
        this.chatgptTestSuccess = false;

        this.loaded = true; // Mark as loaded AFTER data assignment
      } catch (error) {
        console.error("Failed to load configuration:", error);
        this.error = error.message;
      }
    },

    async saveConfiguration() {
      this.saving = true;
      this.error = null;

      try {
        const payload = {
          unbiddenInk: {
            enabled: this.config.unbiddenInk.enabled,
            startHour: this.config.unbiddenInk.startHour,
            endHour: this.config.unbiddenInk.endHour,
            frequencyMinutes: this.config.unbiddenInk.frequencyMinutes,
            prompt: this.config.unbiddenInk.prompt,
          },
        };

        // Include ChatGPT API token if it actually changed vs original
        if (
          (this.config.unbiddenInk.chatgptApiToken || "") !==
          (this.originalValues.chatgptApiToken || "")
        ) {
          payload.unbiddenInk.chatgptApiToken =
            this.config.unbiddenInk.chatgptApiToken;
        }

        await saveConfiguration(payload);

        // Update original values to reflect saved state
        Object.assign(this.originalValues, {
          enabled: this.config.unbiddenInk.enabled,
          startHour: this.config.unbiddenInk.startHour,
          endHour: this.config.unbiddenInk.endHour,
          frequencyMinutes: this.config.unbiddenInk.frequencyMinutes,
          prompt: this.config.unbiddenInk.prompt,
        });

        if (
          (this.config.unbiddenInk.chatgptApiToken || "") !==
          (this.originalValues.chatgptApiToken || "")
        ) {
          this.originalValues.chatgptApiToken =
            this.config.unbiddenInk.chatgptApiToken;
          this.apiTokenModified = false;
        }

        // Redirect immediately with success parameter
        window.location.href = "/settings/?saved=unbiddenInk";
      } catch (error) {
        console.error("Failed to save configuration:", error);
        this.error = error.message;
        this.saving = false; // Only reset on error
      }
    },

    cancelConfiguration() {
      window.location.href = "/settings/";
    },

    // ================== CHANGE DETECTION ==================
    get canSave() {
      const current = this.config.unbiddenInk;
      const original = this.originalValues;

      // Must have no validation errors
      const hasValidationErrors =
        Object.keys(this.validation.errors).length > 0;

      // Detect if token actually changed vs original
      const tokenChanged =
        (current.chatgptApiToken || "") !== (original.chatgptApiToken || "");

      // Other changes (excluding token)
      const otherChanges =
        current.enabled !== original.enabled ||
        current.startHour !== original.startHour ||
        current.endHour !== original.endHour ||
        current.frequencyMinutes !== original.frequencyMinutes ||
        current.prompt !== original.prompt;

      // If token changed and it's still marked as modified,
      // require a successful test (which clears apiTokenModified) before allowing save.
      const blockedByTokenTest = tokenChanged && this.apiTokenModified;

      const hasAnyChange = otherChanges || tokenChanged;

      return hasAnyChange && !hasValidationErrors && !blockedByTokenTest;
    },

    // ================== TOKEN HANDLING ==================
    // Clear API token field on focus (standard UX pattern)
    clearChatgptTokenFieldOnFocus() {
      if (this.config.unbiddenInk.chatgptApiToken) {
        this.config.unbiddenInk.chatgptApiToken = "";
        this.apiTokenModified = true;
        // Invalidate previous test result when user starts editing
        this.chatgptTested = false;
        this.chatgptTestSuccess = false;
      }
    },

    // Token Tracking
    trackChatgptTokenChange(newValue) {
      const hasChanged = newValue !== this.originalValues.chatgptApiToken;
      this.apiTokenModified = hasChanged;
      this.validateChatgptToken(newValue);
      // Any change to the token invalidates prior test status
      if (hasChanged) {
        this.chatgptTested = false;
        this.chatgptTestSuccess = false;
      }
    },

    // ================ Test ChatGPT API Token ================
    chatgptTesting: false,
    async testChatgptConnection() {
      if (!this.config.unbiddenInk.enabled) return;
      const token = this.config.unbiddenInk.chatgptApiToken || "";
      if (!token.trim()) return;
      if (!this.apiTokenModified) return;

      this.chatgptTesting = true;
      try {
        const resp = await fetch("/api/test-chatgpt", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ token }),
        });
        const data = await resp.json().catch(() => ({}));
        if (resp.ok && data && data.success) {
          // Success: clear any token errors and mark as not modified
          if (this.validation.errors["unbiddenInk.chatgptApiToken"]) {
            delete this.validation.errors["unbiddenInk.chatgptApiToken"];
          }
          this.apiTokenModified = false;
          this.chatgptTested = true;
          this.chatgptTestSuccess = true;
        } else {
          const msg =
            (data && (data.error || data.message)) || "Connection test failed";
          this.validation.errors["unbiddenInk.chatgptApiToken"] = msg;
          this.apiTokenModified = false; // lock button until user edits again
          this.chatgptTested = true;
          this.chatgptTestSuccess = false;
        }
      } catch (e) {
        this.validation.errors["unbiddenInk.chatgptApiToken"] =
          "Network error during connection test";
        this.chatgptTested = true;
        this.chatgptTestSuccess = false;
      } finally {
        this.chatgptTesting = false;
      }
    },

    // ================== VALIDATION ==================
    validateChatgptToken(value) {
      if (this.config.unbiddenInk.enabled && (!value || value.trim() === "")) {
        this.validation.errors["unbiddenInk.chatgptApiToken"] =
          "API Token cannot be blank when Unbidden Ink is enabled";
      } else {
        if (this.validation.errors["unbiddenInk.chatgptApiToken"]) {
          delete this.validation.errors["unbiddenInk.chatgptApiToken"];
        }
      }
    },

    // Validate all fields when enabled state changes
    validateAll() {
      this.validateChatgptToken(this.config.unbiddenInk.chatgptApiToken);
    },

    // ================== TIME RANGE MANAGEMENT - DUAL HANDLE SLIDER LOGIC ==================
    get timeRangeDisplay() {
      const start = this.config.unbiddenInk.startHour || 0;
      const end = this.config.unbiddenInk.endHour || 24;

      if (start === 0 && (end === 0 || end === 24)) {
        return "All Day";
      }
      return `${this.formatHour(start)} - ${this.formatHour(end)}`;
    },

    get timeRangeStyle() {
      const start = this.config.unbiddenInk.startHour || 0;
      const end = this.config.unbiddenInk.endHour || 24;

      if (start === 0 && (end === 0 || end === 24)) {
        return { left: "0%", width: "100%" };
      }

      const startPercent = (start / 24) * 100;
      const endPercent = (end / 24) * 100;

      return {
        left: `${Math.min(startPercent, endPercent)}%`,
        width: `${Math.abs(endPercent - startPercent)}%`,
      };
    },

    // Safe getters/setters for slider values (with collision detection)
    get startHourSafe() {
      return this.config.unbiddenInk.startHour || 8;
    },

    set startHourSafe(value) {
      this.config.unbiddenInk.startHour = Math.max(
        0,
        Math.min(24, parseInt(value)),
      );
    },

    get endHourSafe() {
      return this.config.unbiddenInk.endHour || 22;
    },

    set endHourSafe(value) {
      this.config.unbiddenInk.endHour = Math.max(
        0,
        Math.min(24, parseInt(value)),
      );
    },

    // Handle collision-aware start hour changes
    handleStartHourChange(event) {
      const newValue = parseInt(event.target.value);
      const currentEnd = this.config.unbiddenInk.endHour;

      // Prevent start from reaching or exceeding end
      if (newValue >= currentEnd) {
        event.target.value = this.config.unbiddenInk.startHour;
        return;
      }

      this.config.unbiddenInk.startHour = newValue;
    },

    // Handle collision-aware end hour changes
    handleEndHourChange(event) {
      const newValue = parseInt(event.target.value);
      const currentStart = this.config.unbiddenInk.startHour;

      // Prevent end from reaching or going below start
      if (newValue <= currentStart) {
        event.target.value = this.config.unbiddenInk.endHour;
        return;
      }

      this.config.unbiddenInk.endHour = newValue;
    },

    // ================== TIME FORMATTING ==================
    formatHour(hour) {
      if (hour === null || hour === undefined) return "--:--";
      if (hour === 0) return "00:00";
      if (hour === 24) return "24:00";
      return hour.toString().padStart(2, "0") + ":00";
    },

    formatHour12(hour) {
      if (hour === null || hour === undefined) return "--";
      if (hour === 0 || hour === 24) return "12 am";
      if (hour === 12) return "12 pm";
      if (hour < 12) return `${hour} am`;
      return `${hour - 12} pm`;
    },

    // ================== FREQUENCY MANAGEMENT ==================
    get frequencyOptions() {
      return [15, 30, 60, 120, 240, 360, 480]; // minutes
    },

    get frequencyLabels() {
      return this.frequencyOptions.map((minutes) => {
        if (minutes < 60) {
          return `${minutes}min`;
        } else {
          const hours = minutes / 60;
          return `${hours}hr`;
        }
      });
    },

    get frequencySliderValue() {
      const options = this.frequencyOptions;
      const current = this.config.unbiddenInk.frequencyMinutes || 120;
      const index = options.indexOf(current);
      return index >= 0 ? index : 3; // Default to 120min (index 3)
    },

    set frequencySliderValue(index) {
      this.config.unbiddenInk.frequencyMinutes =
        this.frequencyOptions[index] || 120;
    },

    get frequencyDisplay() {
      const minutes = this.config.unbiddenInk.frequencyMinutes || 120;
      const hours = Math.floor(minutes / 60);
      const mins = minutes % 60;
      const start = this.config.unbiddenInk.startHour || 0;
      const end = this.config.unbiddenInk.endHour || 24;

      let timeText = "";
      if (start === 0 && (end === 0 || end === 24)) {
        timeText = "all day long";
      } else {
        const startAmPm = this.formatHour12(start);
        const endAmPm = this.formatHour12(end);
        timeText = `from ${startAmPm} to ${endAmPm}`;
      }

      if (hours > 0 && mins > 0) {
        return `Every ${hours}h ${mins}m ${timeText}`;
      } else if (hours > 0) {
        return `Every ${hours}h ${timeText}`;
      } else {
        return `Every ${mins}m ${timeText}`;
      }
    },

    // ================== QUICK PROMPT MANAGEMENT ==================
    setQuickPrompt(type) {
      const presets = this.config.unbiddenInk.promptPresets || {};
      if (presets[type]) {
        this.config.unbiddenInk.prompt = presets[type];
      }
    },

    isPromptActive(type) {
      const presets = this.config.unbiddenInk.promptPresets || {};
      const currentPrompt = this.config.unbiddenInk.prompt || "";
      return currentPrompt.trim() === (presets[type] || "").trim();
    },

    // ================== UTILITY FUNCTIONS ==================
    showErrorMessage(message) {
      this.error = message;
    },

    // ================== STATUS DISPLAY ==================
    get statusMessage() {
      // When editing (canSave is true), predict message based on current settings
      // When saved (canSave is false), use API status
      if (this.canSave) {
        // Predict based on current hour vs. configured working hours
        const currentHour = new Date().getHours();
        const startHour = this.config.unbiddenInk?.startHour || 8;
        const endHour = this.config.unbiddenInk?.endHour || 22;
        const inWorkingHours =
          currentHour >= startHour && currentHour < endHour;

        if (inWorkingHours) {
          return "Your first Unbidden Ink will unfurl in 1-2 minutes, then follow the rhythm you set.";
        } else {
          return `Your first Unbidden Ink will unfurl when working hours begin (${this.formatHour(startHour)}), then follow the rhythm you set.`;
        }
      } else {
        // Use API status when saved
        const status = this.config.unbiddenInk?.status || "Disabled";
        const startHour = this.config.unbiddenInk?.startHour || 8;

        if (status === "Waiting for working hours") {
          return `Your first Unbidden Ink will unfurl when working hours begin (${this.formatHour(startHour)}), then follow the rhythm you set.`;
        } else if (status === "Active") {
          return "Your first Unbidden Ink will unfurl in 1-2 minutes, then follow the rhythm you set.";
        } else {
          return "";
        }
      }
    },

    get statusBadge() {
      const status = this.config.unbiddenInk?.status || "Disabled";
      const nextScheduled = this.config.unbiddenInk?.nextScheduled || "-";

      if (status === "Active" && nextScheduled !== "-") {
        return {
          icon: "🟢",
          text: `Active: Next in ${nextScheduled}`,
          class:
            "bg-green-50 dark:bg-green-900/20 border-green-200 dark:border-green-800 text-green-800 dark:text-green-200",
        };
      } else if (status === "Waiting for working hours") {
        const start = this.config.unbiddenInk?.startHour || 8;
        const end = this.config.unbiddenInk?.endHour || 22;
        return {
          icon: "🟡",
          text: `Waiting for working hours (${this.formatHour(start)}-${this.formatHour(end)})`,
          class:
            "bg-yellow-50 dark:bg-yellow-900/20 border-yellow-200 dark:border-yellow-800 text-yellow-800 dark:text-yellow-200",
        };
      } else {
        return {
          icon: "⚪",
          text: "Disabled",
          class:
            "bg-gray-50 dark:bg-gray-900/20 border-gray-200 dark:border-gray-800 text-gray-800 dark:text-gray-200",
        };
      }
    },
  };
}
