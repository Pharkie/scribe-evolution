# Settings Refactor Plan 🎯

**Goal:** Modernize and modularize the settings system without breaking functionality.

**Architecture:**
- Bundling constraints require internal code organization (not external modules)
- Alpine.js patterns with proper stores and reactivity
- Page separation with code duplication until Phase 4 bundling improvements
- Incremental approach with continuous testing

## Phase 1: Build System Modernization ⚡ ✅ COMPLETED

- [x] Replace terser with esbuild for 3x faster builds
- [x] Configure proper bundling with tree-shaking and optimization
- [x] Implement dev/prod file naming: `.js` dev, `.min.js` prod
- [x] Rename vendor → alpine for clarity
- [x] Add mock-server debugging support with file mapping
- [x] Remove terser dependency

---

## Phase 2: Internal Organization 🏗️
*Organize code within existing files, prepare for page separation*

### Step 2.0: Section Separation ✅ COMPLETED
- [x] Split Device/WiFi sections in current settings
- [x] Create separate `wifi.html` and `device.html` partials
- [x] Update Alpine store for separated sections
- [x] Verify both sections work in monolithic page

### Step 2.1: Utils Structure ✅ COMPLETED  
- [x] Create `src/js/settings/utils/` directory
- [x] Basic file structure for future use

### Step 2.2: Internal Utilities ✅ COMPLETED
- [x] Extract `showErrorMessage` utility within main store
- [x] Verify internal function extraction works

### Step 2.3: Organize API Functions ✅ COMPLETED
- [x] Group API functions within main store by concern
- [x] Add section headers: DEVICE, WIFI, LED, MQTT, SYSTEM/PRINTING, UTILITIES  
- [x] Verify build and functionality preserved

---

## Phase 3: Complete Page Architecture 📄
*Create individual pages for each settings section*

### Step 3.1: Create Test Page (Device) ✅ COMPLETED
- [x] Create standalone `device.html` page with navigation
- [x] Create focused `page-settings-device.js` Alpine store  
- [x] Copy organized DEVICE CONFIGURATION API functions from main store
- [x] Include UTILITY FUNCTIONS section (showErrorMessage)
- [x] Bundle settings-api with device page (7,145 bytes)
- [x] Update index.html Settings button for testing
- [x] Test complete device functionality: owner, timezone, GPIO pins
- [x] Verify proof of concept for page separation with organized code
- [x] **Testing passed** - Adam manually confirmed Standard Testing Workflow
- [ ] **Live ESP32 testing pending** - Needs Adam to verify on actual hardware

### Step 3.2: Data-Driven Config System: Device Page
**Problem**: Current `/api/config` handler is hardcoded mess - 6 utility functions, manual field mapping, 200+ lines of repetitive validation
**Solution**: Create elegant data-driven configuration system before proceeding to more settings pages
- [x] **Design config field definition system**: Declare fields once with validation rules
- [x] **Create generic validation engine**: Single handler iterates JSON keys, applies rules
- [x] **Eliminate hardcoded field names**: Use reflection/mapping instead of manual switches
- [x] **Consistent partial update pattern**: Any valid key:value updates runtime + NVS automatically
- [x] **Test with device settings page**: Ensure existing functionality preserved
- [x] **Document the pattern**: Clear examples for future settings pages (`docs/DATA_DRIVEN_CONFIG.md`)

### Step 3.3: WiFi Settings Page ✅ COMPLETED
**Problem**: WiFi configuration exists in monolithic settings.html with mixed concerns - single file contains all settings sections and JavaScript  
**Solution**: Extract existing WiFi interface (text fields, layout, functionality) from settings-old.html into dedicated `/settings/wifi.html` with separate `page-settings-wifi.js` Alpine store to enable eventual elimination of monolithic settings.html

- [x] **Frontend implementation complete** - Mock server testing passed
- [ ] **Live ESP32 testing pending** - Needs Adam to verify on actual hardware

### Step 3.4: MQTT Settings Page 🔄 IN PROGRESS
**Problem**: MQTT configuration exists in monolithic settings.html with mixed concerns - single file contains all settings sections and JavaScript
**Solution**: Extract existing MQTT interface (server, port, credentials, enable toggle) from settings-old.html into dedicated `/settings/mqtt.html` with separate `page-settings-mqtt.js` Alpine store to enable eventual elimination of monolithic settings.html

**FOLLOW ESTABLISHED PATTERNS:**
- Use `/settings/mqtt.html` page structure (copy from device.html/wifi.html as templates)
- Create `page-settings-mqtt.js` Alpine store following existing pattern in `page-settings-device.js` and `page-settings-wifi.js`
- Reference existing MQTT functionality in `settings-old.html` for feature requirements
- Must use semantic CSS classes from shared.css - NO long Tailwind strings
- Update build configs in `esbuild.config.js` and `package.json` (follow device/wifi examples)

- [x] Create `/settings/mqtt.html` using data-driven config pattern (reference: `settings-old.html` MQTT section)
- [x] Create `page-settings-mqtt.js` Alpine store (follow page pattern from Step 3.1)
- [x] Implement MQTT connection testing with loading states and timeout handling
- [x] Add server/port/credentials form with validation using existing API endpoints, secure credential handling
- [x] Add enable/disable toggle with connection status feedback and error handling
- [x] Update `esbuild.config.js` with mqtt build config (as we did for device page)
- [x] Update `package.json` build scripts to include mqtt page bundles
- [x] Test against existing MQTT functionality in settings-old.html for feature parity
- [x] **Additional improvements**: Red validation warnings, proper Alpine.js reactivity, test button behavior, no toast messages
- [x] **Frontend implementation complete** - Mock server testing passed
- [ ] **Live ESP32 testing pending** - Needs Adam to verify on actual hardware

#### Step 3.4.5: Timezone Picker Enhancement
**Problem**: Current timezone text input allows invalid IANA timezone strings despite backend validation - users can enter format-correct but invalid timezones like "America/Invalid_City"
**Solution**: Replace text input with searchable timezone picker to eliminate user errors and improve UX
**Approach**: 
- Create separate `/api/timezones` endpoint (not in `/config` - would bloat with 341 entries)
- Implement lazy loading - only fetch when timezone picker is opened
- Add client-side caching after first load
- Create searchable dropdown with friendly display names
- Allow searching by city name, region, or IANA string

**Implementation Plan**:
- [ ] **Data Acquisition**: Download canonical IANA timezone data:
  ```bash
  curl -L "https://nodatime.org/TimeZones?format=json&version=2025b" -o data/resources/timezones.json
  ```
- [ ] **Backend Data Storage**: Timezone data stored as JSON file in `data/resources/timezones.json` with TZDB format:
  ```json
  {
    "ianaVersion": "2025b",
    "fullVersionId": "TZDB: 2025b (mapping: $Revision$)",
    "zones": [
      {
        "id": "Africa/Abidjan", 
        "aliases": ["Africa/Accra", "Iceland"],
        "location": {
          "countryCode": "CI",
          "countryName": "Côte d'Ivoire", 
          "comment": ""
        },
        "currentOffset": "+00 (GMT)"
      }
    ]
  }
  ```
- [ ] **Backend API**: Create `/api/timezones` endpoint with robust error handling:
  - Lazy load: Only reads from LittleFS on first request (memory efficient)
  - Static cache: Store in static String variable for subsequent requests (performance)
  - Error handling: Return 500 if file missing, 500 if JSON malformed, appropriate error messages
  - Memory safety: Monitor heap usage during JSON parsing of large dataset
  - Headers: Add `Cache-Control: public, max-age=86400` and `Content-Type: application/json`
  - Rate limiting: Use existing rate limiting infrastructure
  
- [ ] **Frontend Component**: Replace timezone text input with comprehensive searchable dropdown:
  - **Loading states**: Show spinner while fetching `/api/timezones` on first open
  - **Error handling**: Display error message if API fails, allow retry
  - **Dropdown UI**: Fixed-height scrollable list (max 300px), highlight on hover/keyboard navigation
  - **Keyboard navigation**: Arrow up/down, Enter to select, Escape to close, Tab to next field
  - **Mobile responsive**: Touch-friendly sizing, prevent zoom on input focus
  - **Accessibility**: ARIA labels, screen reader support, proper focus management
  
- [ ] **Search Engine**: Implement comprehensive search with ranking:
  - **Matching logic**: Case-insensitive, partial matching (prefix and contains)
  - **Search fields**: City (IANA ID after `/`, `_` → space), country name, region, aliases, comments
  - **Search examples**:
    - "New York" → "America/New_York" (city extraction with underscore conversion)
    - "United States" → all US timezones (country matching)  
    - "America" → all America/* timezones (region matching)
    - "US/Eastern" → "America/New_York" (alias matching)
    - "Eastern" → "America/New_York" (comment matching)
  - **Result ranking**: Exact matches first, then prefix matches, then contains matches
  - **Performance**: Limit results to top 10, use debounced search (300ms delay)
  
- [ ] **Display Format**: Consistent user-friendly formatting:
  - **Format**: "City, Country (UTC±HH)" - e.g., "New York, United States (UTC-04)"
  - **City extraction**: Split IANA ID on `/`, take last part, convert `_` to spaces
  - **Country**: Use `location.countryName` field
  - **Offset**: Parse `currentOffset` field, clean format to "UTC±HH" 
  - **Fallback**: If city extraction fails, show IANA ID directly
  
- [ ] **Performance Optimization**:
  - **Client caching**: Cache timezone data in memory after first load, persist across page refreshes
  - **Search optimization**: Build searchable index on first load (city/country/alias arrays)
  - **Lazy rendering**: Only render visible dropdown items (virtual scrolling for 341+ items)
  - **Memory cleanup**: Clear search results when dropdown closes
  
- [ ] **Integration & State Management**:
  - **Alpine.js patterns**: Use reactive data, proper x-model binding for selected timezone
  - **Form integration**: Maintain existing validation, change detection, save functionality
  - **Current value handling**: Pre-select current timezone when dropdown opens
  - **Change tracking**: Trigger hasChanges() when selection changes
  
- [ ] **Comprehensive Testing**:
  - **Search functionality**: Test all search types (city, country, region, alias, comment)
  - **Edge cases**: Empty search, no results, malformed timezone data, API failures
  - **Performance**: Test with full 341 timezone dataset, measure search speed
  - **Accessibility**: Screen reader testing, keyboard-only navigation
  - **Mobile**: Touch interaction, dropdown positioning, virtual keyboard handling  
  - **Integration**: Verify save/load works, form validation preserved
  - **Fallback**: Test backend IANA validation catches any invalid selections

**Benefits**: Eliminates timezone input errors, better UX, faster selection, timezone discovery

### Step 3.5: Unbidden Ink Settings Page
**Problem**: Unbidden Ink configuration exists in monolithic settings.html
**Solution**: Extract to dedicated `/settings/unbidden.html` with separate Alpine store
**Risks**: API token security, scheduling validation, autoprompt selection
**Success Criteria**: API token validation, scheduling functionality, autoprompt selection preserved

**Implementation Tasks** (follow **Standard Page Creation Process**):
- [ ] Create `/settings/unbidden.html` and `page-settings-unbidden.js` Alpine store
- [ ] Implement API token validation with secure handling
- [ ] Add scheduling configuration with time validation
- [ ] Add autoprompt selection with preview functionality
- [ ] Run **Testing Workflow**

### Step 3.6: Button Settings Page
**Problem**: Button configuration exists in monolithic settings.html
**Solution**: Extract to dedicated `/settings/buttons.html` with separate Alpine store
**Risks**: Button action validation, testing functionality, action dropdown population
**Success Criteria**: Button action selection, testing functionality, action validation preserved

**Implementation Tasks** (follow **Standard Page Creation Process**):
- [ ] Create `/settings/buttons.html` and `page-settings-buttons.js` Alpine store
- [ ] Implement button action dropdowns with validation
- [ ] Add button testing functionality with feedback
- [ ] Add action validation with user-friendly messages
- [ ] Run **Testing Workflow**

### Step 3.7: LED Settings Page
**Problem**: LED configuration exists in monolithic settings.html
**Solution**: Extract to dedicated `/settings/leds.html` with separate Alpine store
**Risks**: LED effect testing, color picker functionality, brightness control
**Success Criteria**: LED effect testing, color/brightness controls, effect selection preserved

**Implementation Tasks** (follow **Standard Page Creation Process**):
- [ ] Create `/settings/leds.html` and `page-settings-leds.js` Alpine store
- [ ] Implement LED effect selection with preview and testing
- [ ] Add color picker and brightness controls
- [ ] Add effect testing with feedback and error handling
- [ ] Run **Testing Workflow**

### Step 3.8: Memo Settings Page
**Problem**: Memo configuration exists in monolithic settings.html
**Solution**: Extract to dedicated `/settings/memos.html` with separate Alpine store
**Risks**: Content editor functionality, save/load operations, memo management
**Success Criteria**: Memo editor, save/load operations, content management preserved

**Implementation Tasks** (follow **Standard Page Creation Process**):
- [ ] Create `/settings/memos.html` and `page-settings-memos.js` Alpine store
- [ ] Implement memo content editor with validation and character limits
- [ ] Add save/load functionality with error handling
- [ ] Add memo management (create/edit/delete) with user feedback
- [ ] Run **Testing Workflow**

### Step 3.9: Navigation & Cleanup
**Problem**: Settings pages exist in isolation, monolithic files create maintenance burden
**Solution**: Add navigation and remove monolithic files
**Risks**: Navigation breaking, broken links, user confusion, functionality gaps
**Success Criteria**: All pages accessible via navigation, monolithic files removed, no broken links

**Implementation Tasks**:
- [x] Create overview page with section links (`/settings.html` with navigation grid)
- [ ] Add client-side navigation between pages with breadcrumbs and back buttons
- [ ] Verify all individual settings pages are fully functional and tested
- [ ] Remove monolithic settings.html and settings-old.html files
- [ ] Clean up unused partials and legacy code
- [ ] Update all internal links to point to new settings pages
- [ ] Test complete settings workflow end-to-end for regressions
- [ ] Run **Testing Workflow**

---

## Phase 4: Build System Optimization 🚀

- [ ] Fix esbuild multi-entry plugin to support imports
- [ ] Enable proper module separation with external files  
- [ ] Configure code splitting per settings page
- [ ] Create page-specific CSS bundles
- [ ] Add build size monitoring and hot reload
- [ ] Eliminate code duplication through proper imports

---

## Phase 5: Future Planning 📋

- [ ] Document working patterns from settings refactor
- [ ] Create new refactor plan for index, diagnostics, 404 pages
- [ ] Define templates for page structure and API patterns

---

## Process 🛠️

### Standard Page Creation Process
*Follow this process for all new settings pages (Steps 3.5+)*

**Setup Tasks:**
1. Create `/settings/[page].html` using data-driven config pattern
2. Reference existing functionality in `settings-old.html` [section] for feature requirements  
3. Create `page-settings-[page].js` Alpine store (follow established pattern from device/wifi/mqtt pages)
4. Update `esbuild.config.js` with [page] build config 
5. Update `package.json` build scripts to include [page] page bundles

**Development Requirements:**
- Use **Established UI Patterns** and **Alpine.js Form Change Tracking Patterns** (see sections below)
- Follow **CSS Architecture Requirements** - Tailwind utilities only, support light/dark mode
- Test against existing functionality in settings-old.html for feature parity
- Implement proper error handling and validation using existing API endpoints
- Meet **Project Success Criteria** (see Core Principles section below)

**Development Notes:**
- Mock server uses development builds for local testing
- Use standard build commands: `npm run build-js-settings` or `npm run build`
- All builds now use esbuild with consistent output format

### Testing Workflow
1. **Code changes** - Implement functionality
2. **Update build scripts** - If adding new JS files, update package.json build scripts to include them
3. **Build frontend** - `npm run build-js-settings` or `npm run build`
4. **Test with mock server** - `node mock-server/mock-api.js`
5. **Verify functionality** - Test all affected features work correctly
6. **Live device testing** - Invite Adam to deploy to ESP32 and verify on actual hardware (end of phases)
7. **Update REFACTOR_PLAN.md** - Revise and mark steps completed. Revise the plan forward from here based on lessons learned.
8. **Git commit** - Clear commit message with detailed summary
9. **STOP** - Verify 100% before proceeding

### Established UI Patterns (Follow for Phase 3+)
*Based on wifi.html and device.html implementations:*

All new pages must support light/dark mode and use established CSS architecture.

1. **Loading States**: Wait till page ready, fade in with opacity transition. No "Loading" text or spinners.
2. **Visual Hierarchy**: Use accent colors for section headers, not background boxes.
3. **Save Button**: Only enabled when changes detected. Use Alpine reactive `canSave` getter with `hasChanges()` logic.
4. **Tab Interfaces**: Proper tab styling with visual connection to content panel (border, background).
5. **Warning Messages**: Use thin banners above action buttons, not full cards. Left border accent with icon.
6. **Form Change Tracking**: Use proper Alpine.js reactive patterns for immediate field change detection.

### Alpine.js Form Change Tracking Patterns
*Based on MQTT settings page implementation (Step 3.4):*

**✅ DO - Use @input Event Handlers:**
```html
<!-- Server field resets test state immediately on change -->
<input x-model="config.mqtt.server" @input="resetMqttTestState()" />

<!-- Username field with validation and test reset -->
<input x-model="config.mqtt.username" 
       @input="resetMqttTestState()" 
       @blur="validateUsername($event.target.value)" />

<!-- Password field with tracking and test reset -->
<input x-model="config.mqtt.password" 
       @input="trackMqttPasswordChange($event.target.value); resetMqttTestState()" />
```

**✅ DO - Keep Alpine.effect() Simple and Focused:**
```javascript
// Only watch high-level state changes, not individual fields
Alpine.effect(() => {
    if (mqttStore.config?.mqtt?.enabled === false) {
        mqttStore.validation.errors = {};
        mqttStore.resetMqttTestState();
    }
});
```

**❌ DON'T - Custom State Tracking or Manual Watchers:**
```javascript
// WRONG - Manual field watching with variables
let lastServer = mqttStore.config?.mqtt?.server;
Alpine.effect(() => {
    if (mqttStore.config?.mqtt?.server !== lastServer) {
        // Complex comparison logic...
    }
});
```

**Key Benefits:**
- ✅ **Immediate response**: @input handlers fire as user types
- ✅ **Declarative**: Clear HTML shows what triggers what action  
- ✅ **Reactive**: Uses Alpine's built-in reactivity system
- ✅ **Maintainable**: No complex watcher logic or state tracking
- ✅ **Performance**: Direct event handling, no polling or comparisons

### CSS Architecture Requirements

**CRITICAL**: All settings pages must follow Tailwind's utility-first philosophy. No exceptions.

### Required CSS Patterns (per official Tailwind docs):
- **Use utility classes directly in HTML**: Avoid semantic class abstractions
- **Only use @apply for genuine duplication**: Not for creating "semantic" names  
- **Keep styling visible in templates**: Don't hide utilities behind abstracted classes
- **Avoid semantic class names**: Don't create .card or .btn - use utilities directly
- **Light/Dark Mode**: Support both modes using `dark:` prefixes on utilities
- **Complex patterns only**: Use @apply only for webkit/moz patterns or media queries too complex for inline utilities

### Examples:
- ❌ `class="system-action-button-disabled"` (semantic abstraction)
- ✅ `class="bg-gray-200 hover:bg-gray-300 dark:bg-gray-700 dark:hover:bg-gray-600 text-gray-900 dark:text-gray-100 font-medium py-2 px-4 rounded-lg transition-colors duration-200 opacity-50 cursor-not-allowed"` (utilities directly)

## Core Principles
- **One step at a time** - Complete fully before proceeding
- **Ask user** to confirm any major deviations or major decisions before proceeding. Caution. Safety first.
- **Keep old code functional** - Main settings.html works throughout
- **Alpine.js patterns only** - No hacks, use built-in reactivity
- **Fail fast** - No fallback values, let Alpine handle missing data
- **Test everything** - Every endpoint, form, validation
- **Live device testing** - Required at end of each phase for hardware-dependent features

### Project Success Criteria
- All settings functionality identical to current (settings-old.html)
- Code more maintainable and modular
- No regressions in UX, only improvements.
- ESP32 memory usage unchanged

## Lessons Learned 📝
```
**Key Learning:** Always use Alpine's built-in reactivity (`x-effect`, `$nextTick`) instead of custom solutions

**Key Learning:** Maintain parallel versions during major architectural changes

**Key Learning:** Use @input handlers for immediate form field change detection, not complex Alpine.effect() watchers

**Key Learning:** Keep Alpine.effect() simple and focused on high-level state changes, not individual field monitoring
```

## Live ESP32 Testing Required 🔧

### Step 3.1: Device Settings (Pending Hardware Test)
- **Owner field**: Text input with validation
- **Timezone dropdown**: Selection and persistence  
- **GPIO pin configuration**: Hardware-dependent pin validation and assignment
- **Save functionality**: NVS persistence and config reload

### Step 3.3: WiFi Settings (Pending Hardware Test)
- **WiFi scanning**: Hardware radio scanning for available networks
- **SSID/Password configuration**: Network connection establishment
- **Connection timeout**: Hardware timeout behavior
- **Fallback AP mode**: Hardware AP mode activation on connection failure
- **Save functionality**: Network configuration persistence and device restart behavior

### Step 3.4: MQTT Settings (Pending Hardware Test)  
- **MQTT broker connection**: Network connectivity to external MQTT brokers
- **Server/port configuration**: Network socket connection establishment
- **Username/password authentication**: MQTT client authentication
- **Connection testing**: Live broker connectivity verification
- **Enable/disable toggle**: MQTT client service start/stop behavior
- **Save functionality**: MQTT configuration persistence and service restart

**Ready for ESP32 deployment once Adam is available for hardware testing.**

---

## Rejected Ideas 🚫

### Partial Config API Endpoints (3.3 - rejected)
**Problem**: Each settings page needs only its section of config, not full config blob
**Proposed Solution**: Add `/api/config/device`, `/api/config/wifi`, etc. endpoints
**Rejection Reasons**:
- Over-engineering: Full config JSON isn't large enough to warrant optimization
- Memory overhead: Each new endpoint adds routes/handlers on ESP32 with limited resources
- Maintenance burden: 6+ new endpoints to maintain, test, and keep in sync  
- Breaking risk: More complexity = more ways for things to break
**Alternative**: Keep existing `/api/config`, let pages ignore unneeded sections client-side