/**
 * @file diagnostics-logging.js
 * @brief Diagnostics logging page entry point
 * @description Sets up Alpine stores for logging diagnostics
 */

import { createDiagnosticsLoggingStore } from "../stores/diagnostics-logging.js";

// Register Alpine stores when DOM loads
document.addEventListener("alpine:init", () => {
  // Register the diagnostics logging store
  Alpine.store("diagnosticsLogging", createDiagnosticsLoggingStore());
});
