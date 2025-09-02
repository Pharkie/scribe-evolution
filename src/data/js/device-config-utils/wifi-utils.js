/**
 * @file wifi-utils.js
 * @brief Shared WiFi utilities for device configuration pages
 * @description Common WiFi scanning, network management, and validation logic
 * shared between setup.html and settings/wifi.html pages
 */

// No direct API import - scan function will be passed as parameter

/**
 * Format RSSI signal strength to human-readable description
 * @param {number} rssi - Signal strength in dBm
 * @returns {string} Signal strength description
 */
export function formatSignalStrength(rssi) {
  if (rssi > -30) return "Excellent";
  if (rssi > -50) return "Very Good";
  if (rssi > -60) return "Good";
  if (rssi > -70) return "Fair";
  return "Poor";
}

/**
 * Deduplicate WiFi networks by SSID, keeping the strongest signal
 * @param {Array} networks - Array of network objects with ssid and rssi
 * @returns {Array} Deduplicated networks
 */
export function deduplicateNetworks(networks) {
  const networksBySSID = {};

  networks
    .filter((network) => network.ssid && network.ssid.trim())
    .forEach((network) => {
      const ssid = network.ssid.trim();
      if (!networksBySSID[ssid] || network.rssi > networksBySSID[ssid].rssi) {
        networksBySSID[ssid] = network;
      }
    });

  return Object.values(networksBySSID);
}

/**
 * Sort networks by signal strength (strongest first), then alphabetically
 * @param {Array} networks - Array of network objects
 * @returns {Array} Sorted networks
 */
export function sortNetworks(networks) {
  return networks.sort((a, b) => {
    if (b.rssi !== a.rssi) {
      return b.rssi - a.rssi;
    }
    return a.ssid.localeCompare(b.ssid);
  });
}

/**
 * Process raw network scan results into UI-ready format
 * @param {Array} rawNetworks - Raw network scan results
 * @returns {Array} Processed networks with signal strength info
 */
export function processNetworks(rawNetworks) {
  // Deduplicate and sort networks
  const dedupedNetworks = deduplicateNetworks(rawNetworks);
  const sortedNetworks = sortNetworks(dedupedNetworks);

  // Add signal strength information and unique keys
  return sortedNetworks.map((network, index) => ({
    ...network,
    signal_strength: formatSignalStrength(network.rssi),
    signal_display: `${formatSignalStrength(network.rssi)} (${network.rssi} dBm)`,
    uniqueKey: `${network.ssid}-${network.rssi}-${index}`, // Unique key for Alpine rendering
  }));
}

/**
 * Create WiFi scanning state object with reactive properties
 * @param {string} currentSSID - Currently connected SSID
 * @returns {Object} WiFi scanning state object
 */
export function createWiFiState(currentSSID = null) {
  return {
    // Core state
    networks: [],
    currentSSID: currentSSID,
    selectedNetwork: currentSSID,
    manualSSID: "",
    mode: "scan", // 'scan' or 'manual'
    isScanning: false,
    error: null,
    hasScanned: false,
    passwordVisible: false,

    // Computed properties (Alpine getters)
    get sortedNetworks() {
      if (!this.networks || this.networks.length === 0) return [];
      return this.networks; // Networks are already processed and sorted
    },

    // Format signal strength helper
    formatSignalStrength,
  };
}

/**
 * Perform WiFi network scan and update state
 * @param {Object} wifiState - WiFi state object to update
 * @param {Function} showErrorMessage - Error message handler
 * @param {Function} scanFunction - WiFi scanning function to use
 * @returns {Promise<void>}
 */
export async function performWiFiScan(
  wifiState,
  showErrorMessage,
  scanFunction,
) {
  wifiState.isScanning = true;
  wifiState.error = null;

  try {
    const rawNetworks = await scanFunction();
    const processedNetworks = processNetworks(rawNetworks);

    // Update state - Alpine reactivity handles UI updates
    wifiState.networks = processedNetworks;
    wifiState.hasScanned = true;

    // Switch to scan mode and auto-select current network if found
    wifiState.mode = "scan";
    if (wifiState.currentSSID) {
      const currentNetwork = processedNetworks.find(
        (n) => n.ssid === wifiState.currentSSID,
      );
      if (currentNetwork) {
        wifiState.selectedNetwork = wifiState.currentSSID;
        console.log(
          "📡 WiFi Utils: Auto-selected current network:",
          wifiState.currentSSID,
        );
      }
    }

    console.log(
      "📡 WiFi Utils: Scan found",
      processedNetworks.length,
      "networks",
    );
  } catch (error) {
    console.error("📡 WiFi Utils: Scan failed:", error);
    wifiState.error = error.message;
    if (showErrorMessage) {
      showErrorMessage(`WiFi scan failed: ${error.message}`);
    }
  } finally {
    wifiState.isScanning = false;
  }
}

/**
 * Get the effective SSID based on current mode and selection
 * @param {Object} wifiState - WiFi state object
 * @returns {string} Effective SSID
 */
export function getEffectiveSSID(wifiState) {
  if (wifiState.mode === "manual") {
    return wifiState.manualSSID;
  }
  return wifiState.selectedNetwork || "";
}

/**
 * Validate WiFi configuration
 * @param {Object} wifiState - WiFi state object
 * @param {Object} passwordsModified - Password modification tracking (optional)
 * @param {string} password - WiFi password (optional)
 * @returns {Object} Validation result with errors
 */
export function validateWiFiConfig(
  wifiState,
  passwordsModified = null,
  password = null,
) {
  const errors = {};

  const selectedSSID = getEffectiveSSID(wifiState);

  // SSID validation
  if (!selectedSSID || selectedSSID.trim() === "") {
    if (wifiState.mode === "scan") {
      errors["wifi.ssid"] = "Please select a network";
    } else {
      errors["wifi.ssid"] = "Network name cannot be empty";
    }
  }

  // Password validation (if tracking is provided)
  if (
    passwordsModified &&
    passwordsModified.wifiPassword &&
    (!password || password.trim() === "")
  ) {
    errors["wifi.password"] = "Password cannot be blank";
  }

  return {
    isValid: Object.keys(errors).length === 0,
    errors,
  };
}
