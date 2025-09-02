/**
 * @file diagnostics.js
 * @brief Entry point for diagnostics page - imports and registers Alpine.js stores
 * @description Modern ES6 module-based page initialization
 */

import { createDiagnosticsStore } from "../stores/diagnostics.js";

// Register the stores with Alpine.js on initialization
document.addEventListener("alpine:init", () => {
  // Create and register diagnostics store
  const diagnosticsStore = createDiagnosticsStore();
  Alpine.store("diagnostics", diagnosticsStore);

  console.log("✅ Diagnostics Stores registered with ES6 modules");
});
