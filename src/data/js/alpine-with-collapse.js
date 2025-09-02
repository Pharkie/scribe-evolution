/**
 * Alpine.js with Collapse plugin
 * Custom build for Scribe project
 */
import Alpine from "alpinejs";
import collapse from "@alpinejs/collapse";

// Register the collapse plugin
Alpine.plugin(collapse);

// Export for global access (Alpine will auto-start when page loads due to defer attribute)
window.Alpine = Alpine;
