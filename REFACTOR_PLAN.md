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

#### Step 3.4.5: Timezone Picker Enhancement ✅ COMPLETED
**Problem**: Current timezone text input allows invalid IANA timezone strings despite backend validation
**Solution**: Replaced text input with searchable timezone picker 
**Key Implementation**: 
- `/api/timezones` endpoint with lazy loading and static caching
- Searchable dropdown with city/country/region/alias matching  
- Format: "City, Country (UTC±HH)" display with IANA ID storage
- Alpine.js reactive integration with existing form validation
- Full 341 timezone dataset with performance optimization

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

### Step 4.0: REST API Semantics Refactor
**Problem**: Most endpoints were POST even when they only fetch or transform content
**Solution**: Align HTTP methods with REST semantics for proper API design

**HTTP Method Guidelines:**
- GET = safe retrieval or pure transform, no state change
- PATCH = partial updates
- PUT = full replacement of a resource  
- POST = actions with side effects or one-shot triggers

**Corrected Endpoint List:**

**GET Routes (Content Retrieval):**
- `GET /api/character-test` → Get character test pattern content
- `GET /api/joke` → Get random joke content
- `GET /api/news` → Get BBC news headlines
- `GET /api/quiz` → Get random quiz content
- `GET /api/quote` → Get random quote content
- `GET /api/riddle` → Get random riddle content
- `GET /api/poke` → Get poke content
- `GET /api/user-message` → Echo/transform user message for print (e.g., ?text=...)

**PATCH Routes (Partial Updates):**
- `PATCH /api/config` → Partially update configuration

**PUT Routes (Full Replacement):**
- `PUT /api/memo/{id}` → Replace specific memo
- `PUT /api/memos` → Bulk replace all memos

**POST Routes (Actions with Side Effects):**
- `POST /api/led-effect` → Trigger one-shot LED effect
- `POST /api/print-local` → Print custom message locally
- `POST /api/print-mqtt` → Send MQTT message to print
- `POST /api/test-mqtt` → Test MQTT connection
- `POST /api/unbidden-ink` → Trigger unbidden ink

**Implementation Tasks:**
- [ ] Update backend route handlers for new HTTP methods
- [ ] Update frontend code to use correct HTTP methods
- [ ] Update mock server to match new route definitions
- [ ] Update documentation with corrected API specifications
- [ ] Apply URL length limits for GET /api/user-message query parameters
- [ ] Test all endpoints maintain functionality with new methods

**Success Criteria:** All API endpoints follow REST semantics, no functionality regressions, improved API clarity

- [ ] Fix esbuild multi-entry plugin to support imports
- [ ] Enable proper module separation with external files  
- [ ] Configure code splitting per settings page
- [ ] Create page-specific CSS bundles
- [ ] Add build size monitoring and hot reload
- [ ] Eliminate code duplication through proper imports

### Step 4.1: Playwright End-to-End Testing Setup
**Problem**: Need automated testing for Alpine.js + Tailwind frontend to prevent regressions during refactoring
**Solution**: Add Playwright E2E tests that work with existing mock server at http://localhost:3001
**Benefits**: Catch UI/API integration issues early, validate settings pages work correctly, prevent deployment of broken builds

**Implementation Tasks**:
- [ ] **Install dependencies**: Add dev dependencies @playwright/test, dotenv, cross-env
- [ ] **Browser setup**: Run `npx playwright install --with-deps` for browser binaries
- [ ] **Environment config**: Create `.env.test` with `APP_URL=http://localhost:3001`
- [ ] **Playwright config**: Create `playwright.config.js` with testDir="tests", baseURL from env, trace="on-first-retry"
- [ ] **Package scripts**: Add `test:e2e`, `test:e2e:ui`, and `mock` commands to package.json
- [ ] **API tests**: Create `tests/api/status.spec.js` to validate mock server /status endpoint returns {ok: true}
- [ ] **UI smoke tests**: Create `tests/ui/smoke.spec.js` to validate homepage loads and key elements are visible
- [ ] **CI workflow**: Document test execution order (start mock server, run tests in separate shell)
- [ ] **Settings page tests**: Add E2E tests for device/wifi/mqtt settings pages once Step 3 is complete

**Test Coverage Goals**:
- API endpoints return valid JSON responses
- Settings pages load without JavaScript errors
- Form validation works correctly
- Save/load functionality preserves data
- Navigation between settings sections works
- Mobile responsive layout renders correctly

**Success Criteria**: All tests pass consistently, mock server integration works, test suite runs in under 30 seconds

### Critical Issue: CSS File Sizes 🚨
**Problem Discovered**: Tailwind CSS files are 60-80KB each (should be 5-15KB)
- Multiple builds each including large Tailwind 4.x base?
- `settings.css`: 81KB
- `diagnostics.css`: 78KB  
- `index.css`: 65KB
- `setup.css`: 61KB
- `404.css`: 60KB

**Note**: Tailwind 4.x has a large base framework size by design - even minimal builds are 50-80KB

**Investigation Results**:
- ✅ CSS minification works (files are single line)
- ✅ `--minify` flag applied correctly
- ✅ Content detection working as designed
- ℹ️ Tailwind 4.x base size is inherently large (~50-80KB)

**Potential Solutions**:
1. **Single shared CSS build** - One CSS file for all pages instead of 5 separate builds
2. **Manual utility extraction** - Extract only used utilities to custom CSS
3. **Gzip compression** - Compress CSS/JS assets at build time with server decompression

### Gzip Compression Solution 📦
**Concept**: Build-time gzip compression with runtime decompression to reduce filesystem usage by ~70%

**Expected Savings**:
- CSS files: 60-80KB → 15-20KB each (~70% reduction)
- JS files: Alpine.js 64KB → ~18KB (~72% reduction)
- Total filesystem: 1.3MB → ~600-800KB
- **Net result**: Reduced FS partition

**Implementation Requirements**:

**Build Process**:
- Add gzip compression to npm build scripts (using Node.js zlib)
- Generate `.css.gz` and `.js.gz` files alongside originals
- Update file extensions: `index.css` → `index.css.gz`
- Preserve original files for mock-server development

**ESP32 Web Server**:
- Detect `.gz` extensions in file serving logic
- Add `Content-Encoding: gzip` header for compressed files
- Ensure `AsyncWebServer` serves compressed content correctly
- Fallback to uncompressed if decompression fails

**Mock Server Updates**:
- Serve uncompressed originals during development (`.css`, `.js`)
- Add gzip middleware for testing compressed serving (optional)
- Maintain separate dev/prod file serving logic

**Technical Considerations**:
- ESP32-C3 CPU overhead for decompression (minimal for static files)
- Browser compatibility (universal gzip support)
- Build complexity increase
- Development workflow impact (need both compressed/uncompressed)

**Alternative**: Serve pre-compressed files directly without runtime decompression - browsers handle gzip automatically

### Additional Optimization Opportunities 🗜️
**Large Resource Files**:
- `/data/resources/timezones.json` - **174KB** (major space hog)
- `/data/resources/riddles.ndjson` - **111KB** 

**Solutions**:
1. **Gzip compress resources** - `timezones.json.gz` could reduce to ~40KB (70% reduction)
2. **Minify JSON** - Remove whitespace and formatting
3. **Load externally?** - Fetch timezone data from web API instead of local storage (needs evaluation - offline capability, reliability, latency implications)

**Priority**: Address before Phase 5 - current CSS sizes block ESP32 deployment

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

## Recent Critical Issues & Fixes 🔥

### Filesystem Space Crisis (RESOLVED)
**Issue**: ESP32-C3 SPIFFS filesystem full (-28 error) during uploadfs with only 634 bytes free
**Root Cause**: 1.34MB partition too small for 1.3MB data + metadata overhead  
**Solution**: Increased SPIFFS partition from 0x150000 to 0x1F0000 (1.9MB) in `partitions_no_ota.csv`
**Learning**: Always maintain 16%+ headroom for filesystem metadata and wear leveling

### Timezone JSON Corruption (RESOLVED)
**Issue**: "Invalid JSON in timezone data: IncompleteInput" error on `/api/config`
**Root Cause**: `file.readString()` truncated large files on ESP32
**Solution**: Replaced with chunked reading in `api_config_handlers.cpp:loadTimezoneCache()`
```cpp
// Read file in chunks to avoid memory issues with large files
const size_t chunkSize = 1024;
char buffer[chunkSize];
while (file.available()) {
    size_t bytesRead = file.readBytes(buffer, chunkSize);
    cachedTimezoneData += String(buffer).substring(0, bytesRead);
}
```

### Diagnostics API Truncation (RESOLVED)
**Issue**: Diagnostics response truncated due to large route listing in JSON
**Solution**: Created separate `/api/routes` endpoint instead of increasing buffer size
- Reverted diagnostics buffer to 4096 bytes (working within constraints)
- Added `handleRoutes()` in `api_system_handlers.cpp`
- Updated frontend to load routes separately via `DiagnosticsAPI.loadRoutes()`
- Created mock server support with `mock-routes.json`

### Memo Configuration System Fix (RESOLVED)
**Issue**: Memo system bypassed centralized config loader, read NVS directly, causing NOT_FOUND errors on first boot
**Solution**: Integrated memo system with `RuntimeConfig` centralized loader
- Added `memos[4]` array to `RuntimeConfig` structure
- Modified memo handlers to use `g_runtimeConfig.memos[x]` instead of direct NVS reads
- Ensured memo defaults from `config.h` load properly on first boot

## Lessons Learned 📝
```
**Key Learning:** Always use Alpine's built-in reactivity (`x-effect`, `$nextTick`) instead of custom solutions

**Key Learning:** Maintain parallel versions during major architectural changes

**Key Learning:** Use @input handlers for immediate form field change detection, not complex Alpine.effect() watchers

**Key Learning:** Keep Alpine.effect() simple and focused on high-level state changes, not individual field monitoring

**Key Learning:** ESP32 file.readString() fails on large files - use chunked reading for files >8KB

**Key Learning:** Work within buffer constraints - separate endpoints better than large JSON responses

**Key Learning:** Centralized config loading prevents inconsistencies - all settings should use RuntimeConfig
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