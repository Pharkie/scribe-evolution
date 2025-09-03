/**
 * @file diagnostics-microcontroller.js
 * @brief Diagnostics microcontroller page entry point
 * @description Sets up Alpine stores for microcontroller diagnostics
 */

import { createDiagnosticsMicrocontrollerStore } from "../stores/diag-micro.js";

// Register Alpine stores when DOM loads
document.addEventListener("alpine:init", () => {
  // Register the diagnostics microcontroller store
  Alpine.store(
    "diagnosticsMicrocontroller",
    createDiagnosticsMicrocontrollerStore(),
  );
});
