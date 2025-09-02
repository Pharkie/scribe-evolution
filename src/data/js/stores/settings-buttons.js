/**
 * @file settings-buttons.js
 * @brief Alpine.js store factory for button settings page
 * @description Focused Alpine store for button-specific configuration
 * Converted from legacy JavaScript to ES6 modules following established patterns
 */

import { loadConfiguration, saveConfiguration } from "../api/settings.js";

export function createSettingsButtonsStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false, // Simple loading flag (starts false)
    error: null,
    saving: false,
    initialized: false,
    config: {}, // Empty object (populated on load)
    originalConfig: {},
    validationErrors: {},

    // ================== COMPUTED PROPERTIES ==================
    get canSave() {
      return (
        !this.saving &&
        this.loaded &&
        !this.error &&
        this.hasChanges &&
        !this.hasValidationErrors
      );
    },

    get hasChanges() {
      return (
        JSON.stringify(this.config.buttons) !==
        JSON.stringify(this.originalConfig.buttons)
      );
    },

    get hasValidationErrors() {
      return (
        this.validationErrors && Object.keys(this.validationErrors).length > 0
      );
    },

    // ================== BUTTON CONFIGURATION API ==================
    async loadConfiguration() {
      // Duplicate initialization guard (failsafe)
      if (this.initialized) {
        return;
      }
      this.initialized = true;

      this.loaded = false;
      this.error = null;
      try {
        const data = await loadConfiguration();

        // ✅ CRITICAL: Direct assignment to config object
        this.config.buttons = {
          button1: {
            gpio: data.buttons?.button1?.gpio ?? -1,
            shortAction: data.buttons?.button1?.shortAction ?? "",
            longAction: data.buttons?.button1?.longAction ?? "",
            shortMqttTopic: data.buttons?.button1?.shortMqttTopic ?? "",
            longMqttTopic: data.buttons?.button1?.longMqttTopic ?? "",
            shortLedEffect: data.buttons?.button1?.shortLedEffect ?? "none",
            longLedEffect: data.buttons?.button1?.longLedEffect ?? "none",
          },
          button2: {
            gpio: data.buttons?.button2?.gpio ?? -1,
            shortAction: data.buttons?.button2?.shortAction ?? "",
            longAction: data.buttons?.button2?.longAction ?? "",
            shortMqttTopic: data.buttons?.button2?.shortMqttTopic ?? "",
            longMqttTopic: data.buttons?.button2?.longMqttTopic ?? "",
            shortLedEffect: data.buttons?.button2?.shortLedEffect ?? "none",
            longLedEffect: data.buttons?.button2?.longLedEffect ?? "none",
          },
          button3: {
            gpio: data.buttons?.button3?.gpio ?? -1,
            shortAction: data.buttons?.button3?.shortAction ?? "",
            longAction: data.buttons?.button3?.longAction ?? "",
            shortMqttTopic: data.buttons?.button3?.shortMqttTopic ?? "",
            longMqttTopic: data.buttons?.button3?.longMqttTopic ?? "",
            shortLedEffect: data.buttons?.button3?.shortLedEffect ?? "none",
            longLedEffect: data.buttons?.button3?.longLedEffect ?? "none",
          },
          button4: {
            gpio: data.buttons?.button4?.gpio ?? -1,
            shortAction: data.buttons?.button4?.shortAction ?? "",
            longAction: data.buttons?.button4?.longAction ?? "",
            shortMqttTopic: data.buttons?.button4?.shortMqttTopic ?? "",
            longMqttTopic: data.buttons?.button4?.longMqttTopic ?? "",
            shortLedEffect: data.buttons?.button4?.shortLedEffect ?? "none",
            longLedEffect: data.buttons?.button4?.longLedEffect ?? "none",
          },
        };

        this.originalConfig = {
          buttons: JSON.parse(JSON.stringify(this.config.buttons)),
        };

        this.loaded = true; // Mark as loaded AFTER data assignment
      } catch (error) {
        this.error = `Failed to load configuration: ${error.message}`;
      }
    },

    async saveConfiguration() {
      if (!this.canSave) {
        return;
      }

      this.saving = true;
      this.error = null;

      try {
        // Create partial config with ONLY button-specific fields
        const partialConfig = {
          buttons: {
            button1: this.config.buttons.button1,
            button2: this.config.buttons.button2,
            button3: this.config.buttons.button3,
            button4: this.config.buttons.button4,
          },
        };

        console.log("Saving partial button configuration:", partialConfig);
        const message = await saveConfiguration(partialConfig);

        // Success - update original config for buttons section only
        this.originalConfig.buttons = JSON.parse(
          JSON.stringify(this.config.buttons),
        );

        // Redirect immediately to settings overview with success parameter
        window.location.href = "/settings/?saved=buttons";
      } catch (error) {
        console.error("Error saving configuration:", error);
        this.error = error.message || "Failed to save configuration";
        this.showErrorMessage(this.error);
        this.saving = false;
      }
    },

    cancelConfiguration() {
      window.location.href = "/settings/";
    },

    // ================== VALIDATION FUNCTIONS ==================
    isValidMqttTopic(topic) {
      if (!topic) return true; // Empty is valid (means local printing)
      if (topic.length > 65535) return false; // Too long
      if (topic.includes("#") || topic.includes("+")) return false; // No wildcards in publish topics
      if (topic.startsWith("$SYS")) return false; // Reserved prefix
      if (topic.includes("\0")) return false; // No null bytes
      if (topic.trim() !== topic) return false; // No leading/trailing whitespace
      return true;
    },

    validateMqttTopic(topic, buttonNum, pressType) {
      const isValid = this.isValidMqttTopic(topic);
      const errorKey = `mqtt-${buttonNum}-${pressType}`;

      if (!isValid && topic) {
        this.validationErrors = this.validationErrors || {};
        this.validationErrors[errorKey] = "Invalid MQTT topic format";
        return false;
      } else {
        if (this.validationErrors) {
          delete this.validationErrors[errorKey];
        }
        return true;
      }
    },

    // ================== UTILITY FUNCTIONS ==================
    showErrorMessage(message) {
      // Create and show error notification
      const notification = document.createElement("div");
      notification.className =
        "fixed top-4 right-4 bg-red-600 text-white px-6 py-3 rounded-lg shadow-lg z-50 transition-all duration-300";
      notification.textContent = message;
      document.body.appendChild(notification);

      // Auto-remove after 5 seconds
      setTimeout(() => {
        notification.style.opacity = "0";
        setTimeout(() => {
          if (notification.parentNode) {
            notification.parentNode.removeChild(notification);
          }
        }, 300);
      }, 5000);
    },
  };
}
