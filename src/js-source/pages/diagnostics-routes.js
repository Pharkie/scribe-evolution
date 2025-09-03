/**
 * @file diagnostics-routes.js
 * @brief Diagnostics routes page entry point
 * @description Sets up Alpine stores for routes diagnostics
 */

import { createDiagnosticsRoutesStore } from "../stores/diagnostics-routes.js";

// Register Alpine stores when DOM loads
document.addEventListener("alpine:init", () => {
  // Register the diagnostics routes store
  Alpine.store("diagnosticsRoutes", createDiagnosticsRoutesStore());
});
