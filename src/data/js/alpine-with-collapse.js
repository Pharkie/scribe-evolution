/**
 * Alpine.js with Collapse plugin
 * Custom build for Scribe project
 */
import Alpine from "alpinejs";
import collapse from "@alpinejs/collapse";

// Register the collapse plugin
Alpine.plugin(collapse);

// Export for global access
window.Alpine = Alpine;

// Start Alpine (required when loading as ES module)
Alpine.start();
