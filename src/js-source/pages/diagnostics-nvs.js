/**
 * @file diagnostics-nvs.js
 * @brief Diagnostics NVS page entry point
 * @description Sets up Alpine stores for NVS diagnostics
 */

import { createDiagnosticsNvsStore } from "../stores/diagnostics-nvs.js";

// Register Alpine stores when DOM loads
document.addEventListener("alpine:init", () => {
  // Register the diagnostics NVS store
  Alpine.store("diagnosticsNvs", createDiagnosticsNvsStore());
});
