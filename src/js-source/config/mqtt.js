/**
 * MQTT Topic Structure Constants
 * Single source of truth for all MQTT topic patterns
 */

// Base namespace
export const MQTT_NAMESPACE = "scribevolution";

// Resource types
export const MQTT_PRINT_RESOURCE = "print";
export const MQTT_STATUS_RESOURCE = "status";

/**
 * Build print topic: scribevolution/print/{printerName}
 * @param {string} printerName - Human-readable printer name
 * @returns {string} Full MQTT topic
 */
export function buildPrintTopic(printerName) {
  return `${MQTT_NAMESPACE}/${MQTT_PRINT_RESOURCE}/${printerName}`;
}

/**
 * Build status topic: scribevolution/status/{printerId}
 * @param {string} printerId - Unique printer ID (from MAC)
 * @returns {string} Full MQTT topic
 */
export function buildStatusTopic(printerId) {
  return `${MQTT_NAMESPACE}/${MQTT_STATUS_RESOURCE}/${printerId}`;
}

/**
 * Build status subscription wildcard: scribevolution/status/+
 * @returns {string} MQTT subscription pattern
 */
export function buildStatusSubscription() {
  return `${MQTT_NAMESPACE}/${MQTT_STATUS_RESOURCE}/+`;
}
