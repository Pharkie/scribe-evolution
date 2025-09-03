/**
 * @file diagnostics-overview.js
 * @brief Diagnostics overview page entry point
 * @description Sets up Alpine stores for diagnostics navigation
 */

import { createDiagnosticsOverviewStore } from "../stores/diagnostics-overview.js";

// Register Alpine stores when DOM loads
document.addEventListener("alpine:init", () => {
  // Register the diagnostics overview store
  Alpine.store("diagnosticsOverview", createDiagnosticsOverviewStore());
});
