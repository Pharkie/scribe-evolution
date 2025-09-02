/**
 * @file diagnostics-runtime-config.js
 * @brief Diagnostics runtime config page entry point
 * @description Sets up Alpine stores for runtime config diagnostics
 */

import { createDiagnosticsRuntimeConfigStore } from "../stores/diagnostics-runtime-config.js";

// Register Alpine stores when DOM loads
document.addEventListener("alpine:init", () => {
  // Register the diagnostics runtime config store
  Alpine.store(
    "diagnosticsRuntimeConfig",
    createDiagnosticsRuntimeConfigStore(),
  );
});
