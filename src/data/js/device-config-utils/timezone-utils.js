/**
 * @file timezone-utils.js
 * @brief Shared timezone utilities for device configuration pages
 * @description Common timezone loading, search, and display logic
 * shared between setup.html and settings/wifi.html pages
 */

/**
 * Popular timezone defaults (shown first when no search query)
 */
const POPULAR_TIMEZONES = [
  "Europe/London",
  "America/New_York",
  "America/Sao_Paulo",
  "Australia/Sydney",
  "Asia/Tokyo",
];

/**
 * Transform raw timezone data from API into frontend format
 * @param {Array} zones - Raw timezone data from API
 * @returns {Array} Transformed timezone objects
 */
export function transformTimezoneData(zones) {
  return zones.map((zone) => {
    try {
      // Extract city name from IANA ID (after last '/')
      const parts = zone.id ? zone.id.split("/") : ["Unknown"];
      const city = parts[parts.length - 1].replace(/_/g, " ");

      // Create enhanced display name and offset info matching settings-device.js format
      const countryName = zone.location?.countryName;
      const comment = zone.location?.comment?.trim();

      let displayName;
      let offset = "";

      if (
        zone.offsets &&
        Array.isArray(zone.offsets) &&
        zone.offsets.length > 0
      ) {
        // Format offsets with :00 suffix for clarity
        const formatOffset = (o) => {
          const cleaned = o
            .replace(/^\+/, "+")
            .replace(/^-/, "-")
            .replace(/^00/, "+00");
          return cleaned + ":00";
        };

        if (zone.offsets.length === 1) {
          // Single offset (no DST)
          offset = "UTC" + formatOffset(zone.offsets[0]);
          displayName = countryName
            ? `${city}, ${countryName}`
            : zone.id || "Unknown";
          if (comment) {
            displayName += ` — ${comment}`;
          }
        } else {
          // Multiple offsets (DST zone) - first is standard, second is DST
          const standardOffset = formatOffset(zone.offsets[0]);
          const dstOffset = formatOffset(zone.offsets[1]);
          offset = `UTC${standardOffset} / ${dstOffset} DST`;

          displayName = countryName
            ? `${city}, ${countryName}`
            : zone.id || "Unknown";
          if (comment) {
            displayName += ` — ${comment}`;
          }
        }
      } else {
        // Fallback if no offsets available - use currentOffset
        if (zone.currentOffset) {
          const match = zone.currentOffset.match(/([+-]\d{2})/);
          offset = match ? "UTC" + match[1] + ":00" : zone.currentOffset;
        }
        displayName = countryName
          ? `${city}, ${countryName}`
          : zone.id || "Unknown";
      }

      return {
        id: zone.id || "Unknown",
        displayName,
        countryName: countryName || "",
        comment: comment || "",
        aliases: zone.aliases || [],
        offset,
      };
    } catch (transformError) {
      console.error("Error transforming timezone:", zone, transformError);
      return {
        id: zone.id || "Unknown",
        displayName: zone.id || "Unknown",
        countryName: "",
        comment: "",
        aliases: [],
        offset: "",
      };
    }
  });
}

/**
 * Create timezone picker state object
 * @returns {Object} Timezone picker state
 */
export function createTimezoneState() {
  return {
    timezones: [],
    loading: false,
    error: null,
    initialized: false,
  };
}

/**
 * Load timezone data from API
 * @param {Object} timezoneState - Timezone state object to update
 * @returns {Promise<void>}
 */
export async function loadTimezones(timezoneState) {
  if (timezoneState.initialized) return;

  timezoneState.loading = true;
  timezoneState.error = null;

  try {
    const response = await fetch("/api/timezones");
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    const data = await response.json();
    if (!data.zones || !Array.isArray(data.zones)) {
      throw new Error("Invalid timezone data format");
    }

    // Transform timezone data for frontend use
    timezoneState.timezones = transformTimezoneData(data.zones);
    timezoneState.initialized = true;

    console.log(
      `Timezone Utils: Loaded ${timezoneState.timezones.length} timezones`,
    );
  } catch (error) {
    timezoneState.error = `Failed to load timezones: ${error.message}`;
    console.error("Timezone Utils: Loading failed:", error);
  } finally {
    timezoneState.loading = false;
  }
}

/**
 * Get display name for a timezone ID
 * @param {string} timezoneId - Timezone IANA ID
 * @param {Array} timezones - Array of timezone objects
 * @returns {string} Formatted display name
 */
export function getTimezoneDisplayName(timezoneId, timezones) {
  if (!timezoneId) return "";

  const timezone = timezones.find((tz) => tz.id === timezoneId);
  if (timezone) {
    return `${timezone.displayName} (${timezone.offset})`;
  }
  return timezoneId; // Fallback to raw ID
}

/**
 * Get timezone offset for display
 * @param {string} timezoneId - Timezone IANA ID
 * @param {Array} timezones - Array of timezone objects
 * @returns {string} Timezone offset
 */
export function getTimezoneOffset(timezoneId, timezones) {
  if (!timezoneId) return "";

  const timezone = timezones.find((tz) => tz.id === timezoneId);
  return timezone ? timezone.offset : "";
}

/**
 * Filter and search timezones based on query
 * @param {Array} timezones - Array of timezone objects
 * @param {string} searchQuery - Search query string
 * @returns {Array} Filtered and sorted timezone results
 */
export function filterTimezones(timezones, searchQuery = "") {
  if (!Array.isArray(timezones) || timezones.length === 0) {
    return [];
  }

  const query = (searchQuery || "").toLowerCase().trim();

  if (!query) {
    // Show top 5 popular timezones when no search query
    const popular = [];
    const others = [];

    timezones.forEach((timezone) => {
      if (POPULAR_TIMEZONES.includes(timezone.id)) {
        popular.push(timezone);
      } else {
        others.push(timezone);
      }
    });

    // Sort popular by the order in POPULAR_TIMEZONES array
    popular.sort((a, b) => {
      const aIndex = POPULAR_TIMEZONES.indexOf(a.id);
      const bIndex = POPULAR_TIMEZONES.indexOf(b.id);
      return aIndex - bIndex;
    });

    return [...popular, ...others.slice(0, 5 - popular.length)];
  }

  // Search and score results by field priority
  const results = [];

  timezones.forEach((timezone) => {
    let priority = null;

    // Priority 1: City name (extracted from IANA ID)
    const parts = timezone.id.split("/");
    const city = parts[parts.length - 1].replace(/_/g, " ").toLowerCase();
    if (city.includes(query)) {
      priority = 1;
    }
    // Priority 2: Display name
    else if (timezone.displayName.toLowerCase().includes(query)) {
      priority = 2;
    }
    // Priority 3: Timezone ID
    else if (timezone.id.toLowerCase().includes(query)) {
      priority = 3;
    }
    // Priority 4: Country name
    else if (
      timezone.countryName &&
      timezone.countryName.toLowerCase().includes(query)
    ) {
      priority = 4;
    }
    // Priority 5: Comments
    else if (
      timezone.comment &&
      timezone.comment.toLowerCase().includes(query)
    ) {
      priority = 5;
    }

    if (priority !== null) {
      results.push({ timezone, priority });
    }
  });

  // Sort by priority first, then alphabetically by display name
  return results
    .sort((a, b) => {
      if (a.priority !== b.priority) {
        return a.priority - b.priority;
      }
      return a.timezone.displayName.localeCompare(b.timezone.displayName);
    })
    .map((result) => result.timezone);
}

/**
 * Create timezone picker functionality for a store
 * @param {string} searchQuery - Current search query
 * @param {Object} timezoneState - Timezone state object
 * @returns {Object} Timezone picker methods and computed properties
 */
export function createTimezonePicker(searchQuery, timezoneState) {
  return {
    // Load timezones if not already loaded
    async loadTimezones() {
      await loadTimezones(timezoneState);
    },

    // Get display name for a timezone
    getTimezoneDisplayName(timezoneId) {
      return getTimezoneDisplayName(timezoneId, timezoneState.timezones);
    },

    // Get timezone offset
    getTimezoneOffset(timezoneId) {
      return getTimezoneOffset(timezoneId, timezoneState.timezones);
    },

    // Computed property for filtered timezones
    get filteredTimezones() {
      return filterTimezones(timezoneState.timezones, searchQuery);
    },
  };
}

/**
 * Create timezone picker UI interaction methods
 * @param {Object} store - Store object with timezone picker state
 * @returns {Object} UI interaction methods for timezone picker
 */
export function createTimezonePickerUI() {
  return {
    // Load timezones and open dropdown
    async loadTimezonesAndOpen() {
      if (!this.timezonePicker.initialized && !this.timezonePicker.loading) {
        await loadTimezones(this.timezonePicker);
      }
      this.isOpen = true;
    },

    // Open timezone picker (clear search on click/focus)
    async openTimezonePicker() {
      if (!this.timezonePicker.initialized && !this.timezonePicker.loading) {
        await loadTimezones(this.timezonePicker);
      }
      this.searchQuery = "";
      this.isOpen = true;
    },

    // Reset focus index
    resetTimezoneFocus() {
      this.focusedIndex = -1;
    },

    // Handle global keydown to capture typing when dropdown is open
    handleGlobalKeydown(event, refs) {
      // Only handle when dropdown is open and not already focused on search input
      if (!this.isOpen || document.activeElement === refs.searchInput) {
        return;
      }

      // Handle printable characters (letters, numbers, space)
      if (
        event.key.length === 1 &&
        !event.ctrlKey &&
        !event.metaKey &&
        !event.altKey
      ) {
        // Focus search input and add the character
        refs.searchInput.focus();
        // Let the input handle the character naturally
        return;
      }

      // Handle backspace to focus input for editing
      if (event.key === "Backspace") {
        event.preventDefault();
        refs.searchInput.focus();
        // Clear last character if search query exists
        if (this.searchQuery.length > 0) {
          this.searchQuery = this.searchQuery.slice(0, -1);
          this.onSearchInputWithReset();
        }
      }
    },

    // Close timezone picker
    closeTimezonePicker() {
      this.isOpen = false;
      this.focusedIndex = -1;
    },

    // Navigate up in timezone list (Alpine context version)
    navigateTimezoneUp(refs, nextTick) {
      this.focusedIndex = this.focusedIndex > 0 ? this.focusedIndex - 1 : -1;
      if (this.focusedIndex === -1) {
        refs.searchInput.focus();
      } else {
        nextTick(() => {
          const options = refs.dropdown.querySelectorAll(
            "[data-timezone-option]",
          );
          options[this.focusedIndex]?.focus();
        });
      }
    },

    // Navigate down in timezone list (Alpine context version)
    navigateTimezoneDown(refs, nextTick) {
      const maxIndex = Math.min(this.filteredTimezones.length - 1, 4);
      this.focusedIndex =
        this.focusedIndex < maxIndex ? this.focusedIndex + 1 : 0;
      nextTick(() => {
        const options = refs.dropdown.querySelectorAll(
          "[data-timezone-option]",
        );
        options[this.focusedIndex]?.focus();
      });
    },

    // Navigate to first timezone option from input
    navigateToFirstTimezone(refs, nextTick) {
      if (this.isOpen && this.filteredTimezones.length > 0) {
        this.focusedIndex = 0;
        nextTick(() => {
          const options = refs.dropdown.querySelectorAll(
            "[data-timezone-option]",
          );
          options[0]?.focus();
        });
      }
    },

    // Navigate to last timezone option from input
    navigateToLastTimezone(refs, nextTick) {
      if (this.isOpen && this.filteredTimezones.length > 0) {
        this.focusedIndex = Math.min(this.filteredTimezones.length - 1, 4);
        nextTick(() => {
          const options = refs.dropdown.querySelectorAll(
            "[data-timezone-option]",
          );
          options[this.focusedIndex]?.focus();
        });
      }
    },

    // Handle search input changes
    onSearchInput() {
      // Ensure dropdown stays open when typing
      if (!this.isOpen && this.timezonePicker.initialized) {
        this.isOpen = true;
      }
    },

    // Handle search input with focus reset (Alpine-native single method)
    onSearchInputWithReset() {
      this.onSearchInput();
      this.resetTimezoneFocus();
    },

    // Select a timezone
    selectTimezone(timezone) {
      // Set timezone in config - this is store-specific behavior
      if (this.config?.device) {
        this.config.device.timezone = timezone.id;
      }

      this.searchQuery = timezone.displayName;
      this.isOpen = false;

      // Clear validation errors if they exist
      if (this.validation?.errors?.["device.timezone"]) {
        delete this.validation.errors["device.timezone"];
      }

      console.log("Selected timezone:", timezone.id);
    },
  };
}
