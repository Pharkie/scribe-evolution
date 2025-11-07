/**
 * @file settings-leds.js
 * @brief Alpine.js store factory for LED settings page
 * @description Focused Alpine store for LED-specific configuration
 * Converted from legacy JavaScript to ES6 modules following established patterns
 */

import {
  loadConfiguration,
  saveConfiguration,
  triggerLedEffect,
  turnOffLeds,
} from "../api/settings.js";

export function createSettingsLedsStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false, // Simple loading flag (starts false)
    error: null,
    saving: false,
    initialized: false,
    config: {}, // Empty object (populated on load)

    // Effects playground (temporary, not saved to config)
    effect: "chase_single", // Must reflect default option in the select
    speed: 50,
    intensity: 50,
    cycles: 3,
    colors: ["#0062ff"],
    originalConfig: {},
    testingEffect: false,
    // Test button state
    // Label state: 'started' | 'error' | null
    testStatus: null,
    // Visual pressed state (immediate)
    testPressed: false,
    // Fade-back flag (enables transition when returning to default)
    testFading: false,
    // Stop button state
    stopStatus: null, // 'stopped' | 'error' | null
    stopPressed: false,
    stopFading: false,

    // Color presets for swatch selection
    colorPresets: [
      "#0062ff", // Blue
      "#ff0000", // Red
      "#00ff00", // Green
      "#ff00ff", // Magenta
      "#ffff00", // Yellow
      "#00ffff", // Cyan
    ],

    // ================== COMPUTED PROPERTIES ==================
    get canSave() {
      return !this.saving && this.loaded && !this.error && this.hasChanges;
    },

    get hasChanges() {
      // Only check system settings that are actually saved (not playground effects)
      const currentSystemSettings = {
        count: this.config.leds?.count,
        brightness: this.config.leds?.brightness,
      };
      const originalSystemSettings = {
        count: this.originalConfig.leds?.count,
        brightness: this.originalConfig.leds?.brightness,
      };
      return (
        JSON.stringify(currentSystemSettings) !==
        JSON.stringify(originalSystemSettings)
      );
    },

    // ================== LED CONFIGURATION API ==================
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
        this.config.leds = {
          pin: data.leds?.pin ?? -1,
          count: data.leds?.count ?? 30,
          effect: data.leds?.effect ?? "chase_single",
          brightness: data.leds?.brightness ?? 128,
          speed: data.leds?.speed ?? 50,
          intensity: data.leds?.intensity ?? 50,
          cycles: data.leds?.cycles ?? 3,
          colors: data.leds?.colors ?? ["#0062ff", "#ff0000", "#00ff00"],
        };

        this.originalConfig = {
          leds: JSON.parse(JSON.stringify(this.config.leds)),
        };

        // Initialize playground controls from config (keeps UI/state in sync)
        this.effect = this.config.leds.effect || "chase_single";
        this.speed = this.config.leds.speed || 50;
        this.intensity = this.config.leds.intensity || 50;
        this.cycles = this.config.leds.cycles || 3;
        // Derive default colors based on effect
        this.updateEffectParams();

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
        // Create partial config with ONLY LED system settings (exclude effect playground params)
        const partialConfig = {
          leds: {
            count: this.config.leds.count,
            brightness: this.config.leds.brightness,
          },
        };

        console.log("Saving partial LED configuration:", partialConfig);
        const message = await saveConfiguration(partialConfig);

        // Success - update original config for system settings only (not playground effects)
        this.originalConfig.leds = this.originalConfig.leds || {};
        this.originalConfig.leds.count = this.config.leds.count;
        this.originalConfig.leds.brightness = this.config.leds.brightness;

        // Redirect to settings overview with success parameter (no timeout)
        window.location.href = "/settings/?saved=leds";
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

    // ================== COLOR CONTROL FUNCTIONS ==================
    showColorControls() {
      return (
        this.config.leds &&
        (this.effect === "chase_single" ||
          this.effect === "chase_multi" ||
          this.effect === "matrix" ||
          this.effect === "twinkle" ||
          this.effect === "pulse")
      );
    },

    updateEffectParams() {
      // Reset colors based on C++ config requirements
      if (this.effect === "chase_single") {
        this.colors = ["#0062ff"]; // Blue (DEFAULT_CHASE_SINGLE_COLOR)
      } else if (this.effect === "chase_multi") {
        this.colors = ["#ff0000", "#00ff00", "#0000ff"]; // Red, Green, Blue
      } else if (this.effect === "matrix") {
        this.colors = ["#00ff00"]; // Green (DEFAULT_MATRIX_COLOR)
      } else if (this.effect === "twinkle") {
        this.colors = ["#ffff00"]; // Yellow (DEFAULT_TWINKLE_COLOR)
      } else if (this.effect === "pulse") {
        this.colors = ["#800080"]; // Purple (DEFAULT_PULSE_COLOR)
      } else if (this.effect === "rainbow") {
        // Rainbow effect uses no explicit colors
        this.colors = [];
      }
    },

    // ================== LED EFFECT TESTING ==================
    get testButtonLabel() {
      // Two end states: started or error; default label otherwise
      if (this.testStatus === "started") return "LED effect started";
      if (this.testStatus === "error") return "Error";
      return "Test LED Effect";
    },

    get stopButtonLabel() {
      if (this.stopStatus === "stopped") return "Stopped";
      if (this.stopStatus === "error") return "Error";
      return "Stop Test";
    },

    async testLedEffect() {
      if (this.testingEffect) return;

      this.testingEffect = true;
      try {
        // Create effect parameters object (exclude system settings like brightness)
        let colors = this.colors || ["#0062ff"];

        // Only send relevant colors based on C++ config requirements
        if (
          this.effect === "chase_single" ||
          this.effect === "matrix" ||
          this.effect === "twinkle" ||
          this.effect === "pulse"
        ) {
          // Single color effects
          colors = [colors[0]];
        } else if (this.effect === "chase_multi") {
          // Multi color effects need exactly 3 colors
          colors = colors.slice(0, 3);
        } else if (this.effect === "rainbow") {
          // Rainbow ignores explicit colors
          colors = [];
        }

        const effectParams = {
          effect: this.effect,
          speed: this.speed || 50,
          intensity: this.intensity || 50,
          cycles: this.cycles || 3,
          colors: colors,
        };

        console.log("Testing LED effect:", effectParams);

        // Use triggerLedEffect from API
        await triggerLedEffect(effectParams);
        // Success: show pressed state immediately, then fade back
        const fadeMs = 2000;
        this.testStatus = "started"; // label on
        this.testFading = false;
        this.testPressed = true; // pressed style immediately
        setTimeout(() => {
          this.testPressed = false; // remove pressed
          this.testFading = true; // enable fade to default
        }, 0);
        setTimeout(() => {
          this.testFading = false; // fade done
          if (this.testStatus === "started") this.testStatus = null; // reset label
        }, fadeMs);
      } catch (error) {
        console.error("Failed to test LED effect:", error);
        this.showErrorMessage("Failed to test LED effect: " + error.message);
        // Error: show pressed error state immediately, then fade back
        const fadeMs = 2000;
        this.testStatus = "error";
        this.testFading = false;
        this.testPressed = true;
        setTimeout(() => {
          this.testPressed = false;
          this.testFading = true;
        }, 0);
        setTimeout(() => {
          this.testFading = false;
          if (this.testStatus === "error") this.testStatus = null;
        }, fadeMs);
      } finally {
        this.testingEffect = false;
      }
    },

    async turnOffLeds() {
      try {
        console.log("Turning off LEDs");
        await turnOffLeds();
        const fadeMs = 2000;
        this.stopStatus = "stopped";
        this.stopFading = false;
        this.stopPressed = true;
        setTimeout(() => {
          this.stopPressed = false;
          this.stopFading = true;
        }, 0);
        setTimeout(() => {
          this.stopFading = false;
          if (this.stopStatus === "stopped") this.stopStatus = null;
        }, fadeMs);
      } catch (error) {
        console.error("Failed to turn off LEDs:", error);
        this.showErrorMessage("Failed to turn off LEDs: " + error.message);
        const fadeMs = 2000;
        this.stopStatus = "error";
        this.stopFading = false;
        this.stopPressed = true;
        setTimeout(() => {
          this.stopPressed = false;
          this.stopFading = true;
        }, 0);
        setTimeout(() => {
          this.stopFading = false;
          if (this.stopStatus === "error") this.stopStatus = null;
        }, fadeMs);
      }
    },

    // ================== UTILITY FUNCTIONS ==================
    showErrorMessage(message) {
      this.error = message;
    },
  };
}
