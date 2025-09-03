/**
 * Setup API - ES6 module for setup.html page (AP mode initial configuration)
 *
 * This is a simplified version of the settings API focused only on initial device setup
 */

/**
 * Load initial setup configuration from server
 * Uses the dedicated /api/setup endpoint that returns minimal config for AP mode
 * @returns {Promise<Object>} Minimal configuration for setup
 */
export async function loadConfiguration() {
  const response = await fetch("/api/setup", {
    method: "GET",
    headers: {
      "Content-Type": "application/json",
    },
  });

  if (!response.ok) {
    const errorData = await response.json().catch(() => ({}));
    throw new Error(
      errorData.message ||
        `Failed to load setup configuration: ${response.status} - ${response.statusText}`,
    );
  }

  const config = await response.json();
  return config;
}

/**
 * Save setup configuration using the minimal validation endpoint
 * @param {Object} config - Configuration object with device settings
 * @returns {Promise<void>}
 */
export async function saveConfiguration(config) {
  const response = await fetch("/api/setup", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(config),
  });

  if (!response.ok) {
    const errorData = await response.json().catch(() => ({}));
    throw new Error(
      errorData.message ||
        `Failed to save setup configuration: ${response.status} - ${response.statusText}`,
    );
  }
}

/**
 * Scan for available WiFi networks (reuse from main settings API)
 * @returns {Promise<Array>} Array of WiFi network objects
 */
export async function scanWiFiNetworks() {
  // Use AbortController for longer timeout (WiFi scan can take 5-10 seconds)
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), 20000); // 20 second timeout

  const response = await fetch("/api/wifi-scan", {
    method: "GET",
    signal: controller.signal,
  });

  clearTimeout(timeoutId);

  if (!response.ok) {
    const errorData = await response.json().catch(() => ({}));
    throw new Error(
      errorData.message ||
        `WiFi scan failed: ${response.status} - ${response.statusText}`,
    );
  }

  const result = await response.json();
  return result.networks || [];
}

/**
 * Test WiFi credentials without persisting them
 * @param {string} ssid
 * @param {string} password
 * @returns {Promise<{success:boolean,rssi?:number,message?:string}>}
 */
export async function testWiFiConnection(ssid, password) {
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), 20000);

  const response = await fetch("/api/test-wifi", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ ssid, password }),
    signal: controller.signal,
  });

  clearTimeout(timeoutId);

  let data = null;
  try {
    data = await response.json();
  } catch (_) {
    // ignore JSON parse errors; handler should always send JSON
  }

  if (!response.ok) {
    const message = data?.message || `WiFi test failed: ${response.status}`;
    return { success: false, message };
  }

  return data || { success: true };
}
