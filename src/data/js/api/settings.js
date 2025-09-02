/**
 * @file settings.js
 * @brief API communication module - handles all server interactions
 * @description Focused module for loading and saving configuration data
 */

/**
 * Load configuration from server API
 * @returns {Promise<Object>} Configuration object from server
 */
export async function loadConfiguration() {
  try {
    const response = await fetch("/api/config");
    if (!response.ok) {
      throw new Error(
        `Config API returned ${response.status}: ${response.statusText}`,
      );
    }

    const config = await response.json();
    return config;
  } catch (error) {
    console.error("API: Failed to load configuration:", error);
    throw error;
  }
}

/**
 * Save configuration to server API
 * @param {Object} configData - Configuration object to save
 * @returns {Promise<string>} Server response message
 */
export async function saveConfiguration(configData) {
  try {
    const response = await fetch("/api/config", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(configData),
    });

    if (!response.ok) {
      const errorText = await response.text();
      console.error("API: Server error response:", errorText);
      throw new Error(`Server error: ${response.status} - ${errorText}`);
    }

    return "Configuration saved";
  } catch (error) {
    console.error("API: Failed to save configuration:", error);
    throw error;
  }
}

/**
 * Test Unbidden Ink generation via API
 * @param {string} prompt - Prompt text to use for generation
 * @returns {Promise<Object>} Generated content response
 */
export async function testUnbiddenInkGeneration(prompt) {
  try {
    // Build query parameter for custom prompt
    const url = prompt
      ? `/api/unbidden-ink?prompt=${encodeURIComponent(prompt)}`
      : "/api/unbidden-ink";

    const response = await fetch(url, {
      method: "GET",
    });

    if (!response.ok) {
      const errorData = await response.json().catch(() => ({}));
      let errorMessage =
        errorData.error || `HTTP ${response.status}: ${response.statusText}`;

      // Provide more helpful error messages
      if (
        response.status === 500 &&
        errorMessage.includes("Failed to generate")
      ) {
        errorMessage =
          "Failed to generate content. Please check that your ChatGPT API Token is valid and you have sufficient API credits. You can check your account at https://platform.openai.com/account";
      }

      throw new Error(errorMessage);
    }

    return await response.json();
  } catch (error) {
    console.error("API: Failed to test Unbidden Ink:", error);
    throw error;
  }
}

/**
 * Print content locally via API
 * @param {string} content - Content to print
 * @returns {Promise<void>}
 */
export async function printLocalContent(content) {
  try {
    const response = await fetch("/api/print-local", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({ message: content }),
    });

    if (!response.ok) {
      const errorData = await response.json().catch(() => ({}));
      throw new Error(
        errorData.error || `Print failed: HTTP ${response.status}`,
      );
    }
  } catch (error) {
    console.error("API: Failed to print content:", error);
    throw error;
  }
}

/**
 * Trigger LED effect via API with WLED-style parameters
 * @param {string|Object} effectName - Name of the LED effect, or full effect parameters object
 * @param {number} duration - Duration in milliseconds (default 10000) - ignored if effectName is object
 * @param {Object} settings - Effect-specific settings (optional) - ignored if effectName is object
 * @returns {Promise<Object>} API response
 */
export async function triggerLedEffect(
  effectName,
  duration = 10000,
  settings = null,
) {
  try {
    let payload;

    // Handle new WLED-style unified parameters (from Alpine store)
    if (typeof effectName === "object" && effectName.effect) {
      payload = effectName; // Use the full parameters object
    } else if (typeof effectName === "string" && typeof duration === "object") {
      // Handle case where second parameter is the full effect params
      payload = { effect: effectName, ...duration };
    } else {
      // Handle legacy simple parameters
      payload = {
        effect: effectName,
        duration: duration,
      };

      // Add settings to payload if provided
      if (settings && Object.keys(settings).length > 0) {
        payload.settings = settings;
      }
    }

    const response = await fetch("/api/leds/test", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(payload),
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    // LED effect triggered successfully (no response body expected)
    return;
  } catch (error) {
    console.error("API: Failed to trigger LED effect:", error);
    throw error;
  }
}

/**
 * Turn off LEDs via API
 * @returns {Promise<Object>} API response
 */
export async function turnOffLeds() {
  try {
    const response = await fetch("/api/leds/off", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    // LEDs turned off successfully (no response body expected)
    return;
  } catch (error) {
    console.error("API: Failed to turn off LEDs:", error);
    throw error;
  }
}

/**
 * Scan for nearby WiFi networks
 * @returns {Promise<Array>} Array of WiFi networks with SSID, signal strength, and security info
 */
export async function scanWiFiNetworks() {
  try {
    // Use AbortController for longer timeout (WiFi scan can take 5-10 seconds)
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 20000); // 20 second timeout

    const response = await fetch("/api/wifi-scan", {
      signal: controller.signal,
    });

    clearTimeout(timeoutId);
    if (!response.ok) {
      throw new Error(
        `WiFi scan failed: ${response.status} - ${response.statusText}`,
      );
    }

    const result = await response.json();

    // Check for networks array instead of success flag
    if (!result.networks || !Array.isArray(result.networks)) {
      throw new Error("WiFi scan failed - no networks array in response");
    }

    return result.networks;
  } catch (error) {
    console.error("API: Failed to scan WiFi networks:", error);
    throw error;
  }
}

/**
 * Load memos from server API
 * @returns {Promise<Object>} Memos object from server
 */
export async function loadMemos() {
  try {
    console.log("API: Loading memos from server...");

    const response = await fetch("/api/memos");
    if (!response.ok) {
      throw new Error(
        `Memos API returned ${response.status}: ${response.statusText}`,
      );
    }

    const memos = await response.json();
    console.log("API: Memos loaded successfully");
    return memos;
  } catch (error) {
    console.error("API: Failed to load memos:", error);
    throw error;
  }
}

/**
 * Save memos to server API
 * @param {Object} memosData - Memos object to save
 * @returns {Promise<string>} Server response message
 */
export async function saveMemos(memosData) {
  try {
    console.log("API: Sending memos to server...");

    const response = await fetch("/api/memos", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(memosData),
    });

    if (!response.ok) {
      const errorText = await response.text();
      console.error("API: Server error response:", errorText);
      throw new Error(`Server error: ${response.status} - ${errorText}`);
    }

    const result = await response.text();
    console.log("API: Server response:", result);
    return result;
  } catch (error) {
    console.error("API: Failed to save memos:", error);
    throw error;
  }
}
