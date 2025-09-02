/**
 * @file setup.js
 * @brief Entry point for setup page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for AP mode setup
 */

import { createSetupStore } from "../stores/setup.js";

// Register the store with Alpine.js on initialization
document.addEventListener("alpine:init", () => {
  // Create and register setup store
  const setupStore = createSetupStore();
  Alpine.store("setup", setupStore);

  // Create Alpine data component for setup page
  Alpine.data("setupPage", () => ({
    // Expose all store properties as computed getters
    get loaded() {
      return this.$store.setup.loaded;
    },
    get error() {
      return this.$store.setup.error;
    },
    get saving() {
      return this.$store.setup.saving;
    },
    get scanning() {
      return this.$store.setup.scanning;
    },
    get config() {
      return this.$store.setup.config;
    },
    get manualSsid() {
      return this.$store.setup.manualSsid;
    },
    set manualSsid(value) {
      this.$store.setup.manualSsid = value;
    },
    get availableNetworks() {
      return this.$store.setup.availableNetworks;
    },
    get canSave() {
      return this.$store.setup.canSave;
    },

    // Timezone picker properties
    get timezonePicker() {
      return this.$store.setup.timezonePicker;
    },
    get searchQuery() {
      return this.$store.setup.searchQuery;
    },
    set searchQuery(value) {
      this.$store.setup.searchQuery = value;
    },
    get isOpen() {
      return this.$store.setup.isOpen;
    },
    set isOpen(value) {
      this.$store.setup.isOpen = value;
    },
    get focusedIndex() {
      return this.$store.setup.focusedIndex;
    },
    set focusedIndex(value) {
      this.$store.setup.focusedIndex = value;
    },
    get filteredTimezones() {
      return this.$store.setup.filteredTimezones;
    },

    // Expose store methods
    scanWiFi() {
      return this.$store.setup.scanWiFi();
    },
    loadTimezones() {
      return this.$store.setup.loadTimezones();
    },
    getTimezoneDisplayName(timezoneId) {
      return this.$store.setup.getTimezoneDisplayName(timezoneId);
    },
    getTimezoneOffset(timezoneId) {
      return this.$store.setup.getTimezoneOffset(timezoneId);
    },
    selectTimezone(timezone) {
      return this.$store.setup.selectTimezone(timezone);
    },
    saveAndRestart() {
      return this.$store.setup.saveAndRestart();
    },

    init() {
      // Initialize the store
      this.$store.setup.init();
    },
  }));

  // Make setup store instance available globally for the HTML (legacy compatibility)
  window.setupStoreInstance = {
    get loaded() {
      return Alpine.store("setup").loaded;
    },
    get error() {
      return Alpine.store("setup").error;
    },
    get saving() {
      return Alpine.store("setup").saving;
    },
    get scanning() {
      return Alpine.store("setup").scanning;
    },
    get config() {
      return Alpine.store("setup").config;
    },
    get manualSsid() {
      return Alpine.store("setup").manualSsid;
    },
    set manualSsid(value) {
      Alpine.store("setup").manualSsid = value;
    },
    get availableNetworks() {
      return Alpine.store("setup").availableNetworks;
    },
    get canSave() {
      return Alpine.store("setup").canSave;
    },

    // Timezone picker properties
    get timezonePicker() {
      return Alpine.store("setup").timezonePicker;
    },
    get searchQuery() {
      return Alpine.store("setup").searchQuery;
    },
    set searchQuery(value) {
      Alpine.store("setup").searchQuery = value;
    },
    get isOpen() {
      return Alpine.store("setup").isOpen;
    },
    set isOpen(value) {
      Alpine.store("setup").isOpen = value;
    },
    get focusedIndex() {
      return Alpine.store("setup").focusedIndex;
    },
    set focusedIndex(value) {
      Alpine.store("setup").focusedIndex = value;
    },
    get filteredTimezones() {
      return Alpine.store("setup").filteredTimezones;
    },

    scanWiFi() {
      return Alpine.store("setup").scanWiFi();
    },
    loadTimezones() {
      return Alpine.store("setup").loadTimezones();
    },
    getTimezoneDisplayName(timezoneId) {
      return Alpine.store("setup").getTimezoneDisplayName(timezoneId);
    },
    getTimezoneOffset(timezoneId) {
      return Alpine.store("setup").getTimezoneOffset(timezoneId);
    },
    selectTimezone(timezone) {
      return Alpine.store("setup").selectTimezone(timezone);
    },
    saveAndRestart() {
      return Alpine.store("setup").saveAndRestart();
    },
    init() {
      return Alpine.store("setup").init();
    },
  };

  console.log("✅ Setup Store registered with ES6 modules");
});
