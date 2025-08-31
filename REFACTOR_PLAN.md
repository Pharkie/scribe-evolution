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

### Step 3.1: Create First Page (Device) ✅ COMPLETED
- [x] Create standalone `device.html` page with navigation
- [x] Create focused `page-settings-device.js` Alpine store  
- [x] Copy organized DEVICE CONFIGURATION API functions from main store
- [x] Include UTILITY FUNCTIONS section (showErrorMessage)
- [x] Bundle settings-api with device page (7,145 bytes)
- [x] Update index.html Settings button for testing
- [x] Test complete device functionality: owner, timezone, GPIO pins
- [x] Verify proof of concept for page separation with organized code
- [x] **Testing passed** - Adam manually confirmed Standard Testing Workflow
- [x] **Live ESP32 testing** - Needs Adam to verify on actual hardware

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
- [x] **Live ESP32 testing pending** - Needs Adam to verify on actual hardware

### Step 3.4: MQTT Settings Page ✅ COMPLETED
- [x] **Frontend implementation complete** - Mock server testing passed
- [x] **Live ESP32 testing complete** - Adam verified on actual hardware
- [x] MQTT connection testing with timeout handling
- [x] Server/port/credentials form with validation
- [x] Enable/disable toggle with connection status feedback
- [x] Red validation warnings and proper Alpine.js reactivity

#### Step 3.4.5: Timezone Picker Enhancement ✅ COMPLETED  
- [x] **Frontend implementation complete** - Mock server testing passed
- [x] **Live ESP32 testing complete** - Adam verified on actual hardware
- [x] Searchable timezone picker with 341 IANA timezones
- [x] Format: "City, Country (UTC±HH)" with performance optimization

### Step 3.5: Memo Settings ✅ COMPLETED
- [x] **Frontend implementation complete** - Mock server testing passed
- [x] **Live ESP32 testing complete** - Adam verified on actual hardware
- [x] Memo content editor with placeholder system (date, time, timezone, device owner)
- [x] Individual memo editing and bulk save functionality
- [x] Orange theme with proper dark mode support
- [x] Real-time character counter and validation

### Next Steps: Remaining Settings Pages

**CRITICAL**: Follow established patterns from Device/WiFi/MQTT/Memo pages - DO NOT reinvent solutions

### Step 3.6: Button Settings ✅ COMPLETED
- [x] **Frontend implementation complete** - Mock server testing passed
- [x] **Live ESP32 testing pending** - Needs Adam to verify on actual hardware
- [x] Button action configuration for 4 hardware buttons
- [x] Short and long press actions with MQTT topics and LED effects
- [x] Hardware configuration display (read-only)
- [x] Cyan theme with proper dark mode support (updated from indigo)
- [x] **UX improvements**: Grouped Short/Long press sections, MQTT validation, better contrast
- [x] **Icon consistency**: Save button uses checkmark across all settings pages, button icon uses Heroicons circle
- [x] **Partial config architecture**: Only sends button-specific fields, not entire config object
- [x] **Bug fixes**: Fixed double API loading with initialization guard, removed success toast for consistency
- [x] **Color scheme consistency**: All settings pages follow color-coded sections for visual organization

### Step 3.7: LED Settings ✅ COMPLETED
- [x] **Frontend implementation complete** - Mock server testing passed
- [x] **Live ESP32 testing pending** - Needs Adam to verify on actual hardware
- [x] **Pink color scheme**: Using `#db2777` (pink-600) theme matching established patterns
- [x] **LED effect configuration**: Speed, intensity, cycles, colors with live preview testing
- [x] **Hardware validation**: GPIO pin display (configured in Device Settings)
- [x] **Partial config updates**: Only sends system settings (count, brightness, refreshRate)
- [x] **Effects Playground**: Separate testing area with proper color controls per effect type
- [x] **C++ config alignment**: Colors and parameters match led_config.h exactly
- [x] **Save button logic**: Only enables when system settings change (not playground effects)
- [x] **Consistent UX**: Matches save flow, button styling, and validation patterns from other pages

### Step 3.8: Unbidden Ink Settings
- [ ] **Frontend implementation** - Extract AI content configuration 
- [ ] **Purple color scheme**: Use `#9333ea` (purple-600) theme
- [ ] **Content configuration**: Prompts, scheduling, API settings
- [ ] **Partial config updates**: Only send Unbidden Ink-specific fields


### Development Patterns for Steps 3.7-3.8 (MANDATORY)

**Follow Existing Refactored Patterns**:
- **HTML Structure**: Copy from `device.html`, `wifi.html`, `mqtt.html`, `memos.html`, `buttons.html`
- **Alpine.js Stores**: Follow `page-settings-*.js` patterns
- **API Integration**: Use `settings-api.js` patterns with `window.SettingsAPI`
- **CSS Themes**: Use established color schemes (accent colour per section)
- **Form Validation**: Use existing validation patterns and error handling
- **Save/Cancel Flow**: Follow exact patterns from existing pages
- **Partial Config Updates**: CRITICAL - Only send relevant config section, not entire config object

**Mandatory Implementation Steps** (DO NOT DEVIATE):
1. **Copy existing HTML structure** - header, form, validation, save buttons
2. **Create Alpine store** following `page-settings-device.js` patterns
3. **Use existing API functions** from `settings-api.js`
4. **Apply colors** consistently with existing pages
5. **Test with mock server first**, then invite and help user to test ESP32
6. **Update settings overview** with new page links

**Success Criteria**:
- Similar user experience to Device/WiFi/MQTT/Memo pages
- Minimise new patterns introduced - reuse existing solutions or ask the user
- Consistent styling, validation, and save flow
- Full functionality extraction from settings-old.html (do not edit or change that reference file or the settings partials)
- Mock server testing passes before ESP32 testing

### Step 3.9: Legacy Cleanup
**Goal**: Remove outdated monolithic files and finalize modular settings architecture
**Status**: Ready to execute - all individual settings pages are complete and functional

**Current State**: ✅ Modular settings architecture complete
- [x] Device Settings - functional with live ESP32 testing
- [x] WiFi Settings - functional with live ESP32 testing  
- [x] MQTT Settings - functional with live ESP32 testing
- [x] Memo Settings - functional with live ESP32 testing
- [x] Button Settings - functional (mock tested, pending live ESP32)
- [x] Navigation complete - clean Home → Settings → Individual Page hierarchy

**Cleanup Tasks**:
- [ ] Remove `settings-old.html` (legacy monolithic file)
- [ ] Clean up unused partials in `/partials/settings/`
- [ ] Verify no broken internal links remain
- [ ] Final end-to-end testing of complete settings workflow

---

## Phase 4: Build System Optimization 🚀

**4.0 REST API Pattern Standardization** ✅ COMPLETED
- [x] All POST endpoints now consistently follow the correct pattern:
  - **Success**: HTTP 200 + empty body (no JSON parsing)
  - **Error**: Non-200 status + JSON error response (parse for error messages)
- [x] Eliminates JSON parsing errors and creates consistent API behavior
- [x] Content generation endpoints converted from POST to GET (proper REST semantics)
- [x] **Both ESP32 backend AND mock server updated** - full stack consistency
- [x] **Partial Config Pattern**: All settings pages send only relevant config sections, never entire config object

**4.1 Full REST API Compliance** - Convert endpoints to proper HTTP methods:
- **GET** for content generation: `/api/joke`, `/api/riddle`, `/api/quote`, etc. ✅ DONE
- **GET** for reading config: `/api/config` (already correct)
- **PUT** for updating entire config: `/api/config` (currently POST - needs change)
- **PATCH** for partial config updates: `/api/config` (currently POST - consider)
- **POST** for actions only: `/api/print-local`, `/api/print-mqtt`, `/api/setup`
- **DELETE** for resource removal (future possibility)
**4.1 Module System** - Fix esbuild imports, enable code splitting, eliminate duplication
**4.2 CSS Optimization** - Address 60-80KB CSS files, consider gzip compression

### Critical Issue: CSS File Sizes 🚨
**Problem**: Tailwind CSS files are 60-80KB each (Tailwind 4.x base size)
**Solution Options**:
1. Single shared CSS build
2. Gzip compression (70% reduction)
3. Manual utility extraction

**Large Resources**:
- `timezones.json`: 174KB
- `riddles.ndjson`: 111KB

---

## Phase 6: AI Memos (New settings page) 🤖
**Concept**: Dedicated AI memo generation with hardware button integration

### Step 6.1: AI Memos Configuration
- [ ] **AI Memos section**: 4 configurable AI prompts (e.g. "Daily reflection", "Creative writing", "Problem solving", "Random inspiration")
- [ ] **Hardware button integration**: Each prompt assignable to physical buttons for instant generation
- [ ] **System-level ChatGPT configuration**: Move ChatGPT API token from Unbidden Ink to Device settings as shared system resource
- [ ] **Separation of concerns**: Scheduled AI content (Unbidden Ink) vs on-demand AI content (AI Memos)
- [ ] **Color scheme**: TBD - likely distinct from existing pages
- [ ] **Integration**: Coordinate with Button settings for AI memo action assignment

---

## Phase 5: Core Pages Refactor 📄

**Goal**: Apply proven settings refactor patterns to remaining pages
**Approach**: Use established patterns from Phase 3 settings pages as templates

### Step 5.1: Index Page Refactor
**Current State**: Monolithic `index.html` with embedded Alpine store
**Target**: Modular architecture following settings patterns
- **HTML**: Clean separation of concerns, component-based structure
- **JS**: Extract to `page-index.js` following `page-settings-*.js` patterns  
- **API**: Consolidate API calls using established `*-api.js` patterns
- **Theme**: Maintain existing design while improving code organization

### Step 5.2: Diagnostics Page Refactor  
**Current State**: Functional but should follow established patterns better
Split up from monolithic like settings.html.
**Target**: Align with settings architecture patterns and modularize diagnostic sections

**Sub-phases**:
- **5.2.1 Microcontroller** - Hardware, firmware, memory diagnostics
- **5.2.2 Logging** - System logs
- **5.2.3 Routes** - Pages and API endpoints
- **5.2.4 Runtime Config** - Active configuration display and validation  
- **5.2.5 NVS** - Non-volatile storage (NVS) inspection

**Implementation**:
- **HTML**: Review and align with settings page structure, modularize sections
- **JS**: Ensure `page-diagnostics.js` follows established patterns
- **API**: Standardize API integration patterns  
- **Validation**: Apply settings-style validation and error handling
- **Partials**: Consider extracting diagnostic sections to reusable components

### Step 5.3: 404 Page Refactor
**Current State**: Basic functionality
**Target**: Polish and align with overall architecture
- **HTML**: Consistent styling and structure
- **JS**: Minimal but following patterns where applicable
- **UX**: Improve navigation and user guidance

### Step 5.4: Documentation & Templates
- [ ] Document proven patterns from settings and core page refactors  
- [ ] Create reusable templates for future page development
- [ ] Establish coding standards and architectural guidelines

---

## Process 🛠️

### Process
**Standard Workflow**: Create page → Build → Mock test → Live ESP32 test → Update plan → Commit

### Development Patterns
- **Alpine.js**: Use @input handlers for immediate changes, keep Alpine.effect() simple
- **CSS**: Tailwind utilities directly in HTML, support light/dark mode, avoid semantic abstractions
- **Forms**: Save button enabled only when changes detected
- **Testing**: Mock server → Live ESP32 verification

### CSS Requirements
- Use Tailwind utility classes directly in HTML (not semantic abstractions like `.btn`)
- Support light/dark mode with `dark:` prefixes
- Only use `@apply` for genuine duplication or complex webkit patterns
- Keep styling visible in templates

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

**Alpine.js Patterns:**
- Use built-in reactivity (`x-effect`, `$nextTick`) not custom solutions
- Use @input handlers for immediate form changes, not complex watchers
- Keep Alpine.effect() simple and focused on high-level state changes
- **Dark mode**: Use Tailwind classes `dark:` not inline styles (timezone dropdown fix pattern)
- **Save button consistency**: Always use `canSave` getter with `hasChanges` logic across all settings pages

**Alpine Store Initialization Pattern:**
```javascript
// ✅ CORRECT: Initialize AFTER DOM binding is established
function createMyStore() {
    return {
        loading: false,
        ready: false,
        data: {},
        
        async loadData() {
            this.loading = true;
            // ... load data ...
            this.ready = true;
            this.loading = false;
        }
    };
}

document.addEventListener('alpine:init', () => {
    Alpine.store('myStore', createMyStore()); // No immediate initialization
});
```

```html
<!-- HTML: Initialize after Alpine establishes DOM binding -->
<body x-data="$store.myStore" x-init="$nextTick(() => loadData())">
    <template x-if="!loading && !error && ready">
        <!-- Content shows after async load completes -->
    </template>
</body>
```

```javascript
// ❌ ANTIPATTERN 1: Initialize during store creation (timing issue)
function createMyStore() {
    const store = { /* methods */ };
    
    // ❌ BAD: Alpine hasn't established DOM reactivity yet
    store.loadData(); // State changes invisible to Alpine
    
    return store;
}
```

```javascript
// ❌ ANTIPATTERN 2: Custom init() method causes double initialization  
function createMyStore() {
    return {
        async init() { /* Gets called multiple times */ },
        /* other methods */
    };
}

document.addEventListener('alpine:init', () => {
    const store = createMyStore();
    Alpine.store('myStore', store);
    store.init(); // ← Manual call + Alpine auto-call = double initialization
});
```

**Alpine Event Listener Pattern**: Use `document.addEventListener('alpine:init', ...)` which attaches to Alpine's init event on the document object. This is what Alpine officially documents and is the canonical pattern.

**Critical Timing Rule**: Store initialization MUST happen AFTER Alpine establishes DOM binding (`x-data`), not during store creation or registration. Use `x-init="$nextTick(() => loadData())"` to ensure proper timing.

**Smooth Fade-In Transitions Pattern:**
```html
<!-- ✅ CORRECT: x-show + x-transition + style="display: none" to prevent FOUC -->
<div x-show="!loading && !error" x-transition style="display: none">
    <form><!-- Content --></form>
</div>
```

**FOUC Prevention**: The `style="display: none"` is CRITICAL to prevent Flash of Unstyled Content (FOUC). Without it, content briefly appears before Alpine.js loads and applies the `x-show` logic.

```javascript
// ✅ CORRECT: Start with loading: true for fade-in effect
function createMyStore() {
    return {
        loading: true,  // Hides content initially
        ready: false,
        
        async loadData() {
            this.loading = true;
            // ... load data ...
            this.ready = true;
            this.loading = false;  // Triggers fade-in via x-show
        }
    };
}
```

```html
<!-- ❌ WRONG: x-if prevents transitions (removes/recreates DOM) -->
<template x-if="!loading && !error">
    <div x-transition><!-- Won't work - element doesn't exist for transition --></div>
</template>
```

**Key Points:**
- **Use `x-show`** (toggles visibility) NOT `x-if` (removes DOM elements)
- **Start with `loading: true`** to hide content initially  
- **Alpine's `x-transition`** provides automatic fade-in when `x-show` becomes true
- **Apply to ALL settings pages** for consistent UX

**CRITICAL: Initialize Config Structure Pattern:**
```javascript
// ✅ CORRECT: Initialize ALL data structures that Alpine expressions will access
function createMyStore() {
    return {
        loading: true,
        config: {
            // Initialize EXACT structure needed by HTML expressions with null values
            buttons: {
                button1: { gpio: null, shortAction: null, longAction: null },
                button2: { gpio: null, shortAction: null, longAction: null },
                // ... all fields that HTML will access
            },
            device: {
                owner: null,
                timezone: null,
                printerTxPin: null
            }
            // ... other sections as needed
        },
        
        async loadConfiguration() {
            // This overwrites the null values with real data from server
        }
    };
}
```

```javascript
// ❌ WRONG: Empty config object causes "Cannot read properties of undefined" errors
config: {},  // Alpine expressions like config.buttons.button1 will fail immediately
```

**Data Structure Rule**: Alpine.js tries to evaluate ALL expressions immediately when the store binds to HTML. If your HTML contains expressions like `config.buttons.button1.gpio`, the ENTIRE path (`config.buttons.button1`) must exist in the store's initial state, even with `null` values. The structure gets populated with real data when `loadConfiguration()` completes.

**ESP32 Constraints:**
- file.readString() fails on large files - use chunked reading for >8KB
- Work within buffer constraints - separate endpoints better than large JSON
- Don't overcomplicate native ESP32 functionality (WiFi scanning) - it worked before
- Single WiFi radio means scanning affects connection - let ESP32 handle natively
- MQTT TLS: Racing connections cause socket conflicts - use connection guard and recreate WiFiClientSecure

**Configuration Management:**
- Centralized config loading prevents inconsistencies - all settings use RuntimeConfig
- Maintain parallel versions during major architectural changes
- **C++ config alignment critical**: Frontend defaults and parameters must exactly match C++ led_config.h constants
- **Mock server data quality**: Ensure slider step alignment (step="5" requires multiples of 5 values)

**UI/UX Patterns:**
- **Separate system vs playground settings**: Distinguish between saved configuration and temporary testing features
- **Visual hierarchy**: Use clear section separation and explanatory text for different functional areas
- **Icon semantics**: Use beaker icon for testing/experimentation, megaphone for broadcasting/MQTT

---

## Rejected Ideas 🚫

**Partial Config GET Endpoints** - Creating `/api/config/device`, `/api/config/wifi` etc. for reading config sections. Over-engineering: full config JSON isn't large enough to warrant splitting, adds maintenance burden and memory overhead on ESP32. (Note: partial writes via POST `/api/config` already work fine.)

## Live ESP32 Testing Status 🔧

**✅ COMPLETED - Step 3.1: Device Settings**
**✅ COMPLETED - Step 3.3: WiFi Settings** 
**✅ COMPLETED - Step 3.4: MQTT Settings**
**✅ COMPLETED - Step 3.4.5: Timezone Picker**
**✅ COMPLETED - Step 3.5: Memo Settings**
**✅ COMPLETED - Step 3.6: Button Settings** (mock tested, pending live ESP32)
**✅ COMPLETED - Step 3.7: LED Settings** (mock tested, pending live ESP32)  
**✅ COMPLETED - Step 3.8: Unbidden Ink Settings** (mock tested, pending live ESP32)

**🔧 NEXT: Live ESP32 Hardware Testing**
All settings pages have been mock tested and are ready for live ESP32 hardware verification:
- Button Settings: Alpine.js initialization fixed, config structure properly initialized
- LED Settings: Alpine.js initialization fixed, playground values separated from config
- Unbidden Ink Settings: Alpine.js initialization fixed, formatHour functions made null-safe

**Critical Alpine.js Pattern Applied**: All settings pages now use the consistent initialization pattern:
- `document.addEventListener('alpine:init')` (Alpine.js canonical pattern)
- Config structures pre-initialized with null values to prevent "Cannot read properties of undefined" errors
- Initialization guards to prevent duplicate calls
- Null-safe functions for all computed properties that handle initial null values

**⚡ IMPROVED: Simple Loading Flag Pattern (Applied to diagnostics.html)**

The original pre-initialized null structures pattern was fragile and required exact API structure matching. A better pattern emerged:

**1. Replace Pre-Initialized Structures with Simple Loading Flag:**
```javascript
// ❌ FRAGILE: Pre-initialized null structures
diagnosticsData: {
  microcontroller: {
    chip_model: null,
    cpu_frequency_mhz: null,
    flash: { /* complex nested null structure */ }
  }
}

// ✅ SIMPLE: Just use loading flag + empty data object
loaded: false,
data: {}
```

**2. Replace 'loading' with 'loaded' Flag:**
```javascript
// ❌ REDUNDANT: Multiple loading states
loading: true,    // for API calls in progress
ready: false,     // for data availability 
error: null

// ✅ CLEAN: Single state flag
loaded: false,    // false = loading/not ready, true = data available
error: null       // still need error state
```

**3. Update Getters to Check Loading State:**
```javascript
// ✅ All getters follow same pattern
get someData() {
  if (!this.loaded) return null;  // or return [] for arrays
  
  const data = this.data.someApi?.someField;
  if (!data) {
    console.error('❌ Missing data from API');
    return 'ERROR: Missing Data';
  }
  return data;
}
```

**4. Update HTML Templates:**
```html
<!-- ❌ OLD: Multiple flags -->
<div x-show="!loading && !error && ready">

<!-- ✅ NEW: Simple flag -->
<div x-show="loaded && !error">
```

**5. API Loading Pattern:**
```javascript
async loadData() {
  this.loaded = false;
  this.error = null;
  
  try {
    // Load all APIs
    this.data.someApi = await fetchSomeData();
    this.data.otherApi = await fetchOtherData();
    
    this.loaded = true;
  } catch (error) {
    this.error = error.message;
  }
}
```

**Benefits of Simple Loading Flag Pattern:**
- **Less Brittle**: No need to pre-define exact API structure
- **Backend Flexible**: API can change structure without breaking frontend initialization  
- **Cleaner Code**: ~20 lines vs ~50+ lines of complex null initialization
- **Better Separation**: Loading state vs data access clearly separated
- **Future-Proof**: Adding new API fields won't break existing code
- **Maintains Alpine Patterns**: Still uses canonical Alpine.js initialization

**When to Apply This Pattern:**
- Pages with complex nested API data structures
- Pages consuming multiple API endpoints
- Any page where API structure might change
- Diagnostics-type pages with dynamic data requirements

**Implementation Checklist:**
1. Replace pre-init structures with `loaded: false, data: {}`
2. Remove redundant `loading`/`ready` flags, keep just `loaded`
3. Update all getters: `if (!this.loaded) return null/[]`
4. Update HTML: `x-show="loaded && !error"`
5. Set `this.loaded = true` after successful API calls
6. Test that page shows loading state, then content, handles errors

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