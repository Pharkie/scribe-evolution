/**
 * @file diagnostics-routes.js
 * @brief Alpine.js store factory for diagnostics routes page
 * @description Focused Alpine store for API routes and request statistics
 */

import { loadRoutes } from "../api/diagnostics.js";

/**
 * Create Diagnostics Routes Alpine Store
 * Contains API endpoints, request statistics, and route information
 */
export function createDiagnosticsRoutesStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false,
    error: null,
    routes: {},

    // ================== INITIALIZATION ==================
    async loadData() {
      this.loaded = false;
      this.error = null;
      try {
        const routes = await loadRoutes();

        // Transform the routes data to include computed properties expected by the template
        const apiEndpoints = routes.api_endpoints || [];
        const webPages = routes.web_pages || [];

        // Sort endpoints by method (GET first, then POST) and alphabetically within each group
        const sortedEndpoints = [...apiEndpoints].sort((a, b) => {
          const methodA = (a.method || "").toUpperCase();
          const methodB = (b.method || "").toUpperCase();

          // Define method priority (GET first, then POST, then others alphabetically)
          const methodPriority = { GET: 1, POST: 2 };
          const priorityA = methodPriority[methodA] || 999;
          const priorityB = methodPriority[methodB] || 999;

          // First sort by method priority
          if (priorityA !== priorityB) {
            return priorityA - priorityB;
          }

          // If same method priority, sort alphabetically by path
          return (a.path || "").localeCompare(b.path || "");
        });

        this.routes = {
          ...routes,
          totalRoutes: apiEndpoints.length + webPages.length,
          apiRoutes: apiEndpoints.length,
          staticRoutes: webPages.length,
          endpoints: sortedEndpoints, // Use sorted endpoints for the template
          totalRequests: 0, // ESP32 doesn't track this currently
          statistics: {
            successfulRequests: 0,
            clientErrors: 0,
            serverErrors: 0,
            averageResponseTime: 0,
          },
        };

        this.loaded = true;
      } catch (error) {
        this.error = `Failed to load routes data: ${error.message}`;
        this.loaded = true;
      }
    },

    // ================== UTILITY FUNCTIONS ==================
    getMethodClass(method) {
      switch (method?.toUpperCase()) {
        case "GET":
          return "bg-blue-100 text-blue-800 dark:bg-blue-900 dark:text-blue-300";
        case "POST":
          return "bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-300";
        case "PUT":
          return "bg-yellow-100 text-yellow-800 dark:bg-yellow-900 dark:text-yellow-300";
        case "DELETE":
          return "bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-300";
        case "PATCH":
          return "bg-purple-100 text-purple-800 dark:bg-purple-900 dark:text-purple-300";
        default:
          return "bg-gray-100 text-gray-800 dark:bg-gray-700 dark:text-gray-300";
      }
    },
  };
}
