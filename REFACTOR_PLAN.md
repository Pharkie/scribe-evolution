# App Refactor Plan 🎯

**Goal:** Modernize and modularize the app without breaking functionality.

### Phase 1: Build System Modernization ⚡

- [x] esbuild integration (3x faster builds)
- [x] Dev/prod file naming (`.js`/`.min.js`)
- [x] Mock server support

### Phase 2: Internal Organization 🏗️

- [x] Section separation (Device/WiFi partials)
- [x] API function organization within main store
- [x] Utility structure preparation

## Phase 3: Settings refactor ✅

**Key Features**:

- Color-coded themes per section
- Partial config updates (send only relevant fields)
- Consistent Alpine.js patterns and error handling
- Mock server testing + live ESP32 verification
- Navigation hierarchy: Home → Settings → Individual Pages

**Status**: All individual settings pages complete, ready for cleanup

### Settings Pages (3.1-3.8) - Completed

- **Device** (blue) - Owner, timezone, GPIO pins, data-driven config system
- **WiFi** (green) - Network credentials, connection status, Simple Loading Flag Pattern ✅
- **MQTT** (red) - Server config, connection testing, enable/disable
- **Memo** (orange) - Content editor, placeholders, character counter
- **Button** (cyan) - Hardware actions, short/long press, MQTT integration
- **LED** (pink) - Effects config, playground testing, C++ alignment
- **Unbidden Ink** (purple) - AI prompts, scheduling, API settings

**Testing Status**: Live ESP32 tested and working ✅ All routing, compression, and API fixes verified in production

### Step 3.9: Legacy Cleanup - Completed

- [x] Remove `settings-old.html` (legacy monolithic file)
- [x] Clean up unused partials in `/partials/settings/`
- [x] Verify no broken internal links remain
- [x] Final end-to-end testing of complete settings workflow

---

## Phase 4: Optimization 🚀

**Summary**: Major optimization opportunity identified in CSS architecture.

### Completed Optimizations

- **REST API Standardization** ✅ - Success: HTTP 200 + empty body, Error: Non-200 + JSON
- **Partial Config Pattern** ✅ - Settings pages send only relevant sections

### 4.1: Large File Analysis ✅

- **timezones.json (174KB)**: Required as-is - contains essential IANA timezone data
- **riddles.ndjson (111KB)**: Required as-is - complete content library needed
- **Assessment**: Large files are necessary and optimally structured

### 4.2: CSS Purging Investigation ✅

- **Fixed Tailwind content paths**: `./data/html/**/*.html` → `./data/**/*.html`
- **Confirmed purging works**: Tested with unused classes, purging active and functional
- **Assessment**: CSS classes are already optimally purged - no unused classes found

### 4.3: CSS Architecture Improvement - ✅ COMPLETED

**Problem Discovered**: Each page duplicates full Tailwind base (~90KB)

- `settings.css`: 123KB (90KB base + 33KB components)
- `diagnostics.css`: 121KB (90KB base + 31KB components)
- `index.css`: 103KB (90KB base + 13KB components)
- **Total**: 541KB with ~300KB duplication

**Solution Implemented**: ✅ Single comprehensive `app.css` file

- Created `src/css/app.css` with `@import "tailwindcss";` and all component styles
- Consolidated all custom styles from 5 CSS files into organized sections
- Updated build process: `npm run build-css` → single CSS build
- Updated all HTML templates to load `/css/app.css`
- Updated package.json build scripts

**Results Achieved**: 🎯 **76.9% size reduction**

- **Original**: 541KB total (5 separate files)
- **New unminified**: 171KB (single app.css)
- **New minified**: 125KB (production app.css)
- **Final savings**: 417KB reduction from original

**Benefits Realized**:

- ✅ Single cached file - browser caches once, benefits all pages
- ✅ Eliminated duplication - no repeated Tailwind base
- ✅ Faster builds - one CSS build instead of 5 parallel builds
- ✅ Better performance - 77% smaller CSS payload
- ✅ Easier maintenance - all styles centrally organized
- ✅ Proper Tailwind architecture - content scanning across all files

### 4.4: GZIP Compression - ✅ COMPLETED

**Results Achieved**: 🎯 **76.3% average bandwidth reduction**

- **app.css**: 125KB → 17KB (86% reduction)
- **index.html**: 42KB → 7KB (83% reduction)
- **page-index.js**: 29KB → 6KB (79% reduction)
- **alpine.js**: 45KB → 16KB (64% reduction)
- **Total assets**: 1.7MB → 288KB (83% reduction!)

**Implementation Completed**: ✅ Pre-compressed assets with simplified serving

- **Build system**: `npm run gzip-assets` creates `.gz` versions (46 files compressed)
- **Mock server**: Serves compressed files with `Content-Encoding: gzip` headers
- **No fallbacks**: All modern browsers support GZIP (universal since IE6+)
- **Smart compression**: Files <1KB skipped (overhead > benefit)

**Benefits Realized**:

- ✅ **Massive bandwidth savings** - 83% reduction across all web assets
- ✅ **Faster page loads** - especially beneficial over slower WiFi
- ✅ **Reduced ESP32 power usage** for WiFi transmission
- ✅ **Better caching** - 1-year cache for compressed assets
- ✅ **Build integration** - automatically runs with `npm run build-prod`

**ESP32 Implementation Ready**: 📋 AsyncWebServer API confirmed

- Use `server.serveStatic("/file", LittleFS, "/file.gz").setContentEncoding("gzip")`
- Explicit `Content-Encoding: gzip` header setting (no auto-detection)
- Long-term caching with `setCacheControl("max-age=31536000")`
- Estimated ~2x SPIFFS storage usage (manageable on 4MB flash)

**Next Steps**: ESP32 web server implementation when ready

### 4.5: ES6 Module System ✅ COMPLETED

**Conversion Process**: ES6 modules in source → IIFE bundles for ESP32

**Per-Page Conversion Steps**:

1. **Create API module**: Move API functions to `src/js/api/{page}.js` with ES6 exports
2. **Create store module**: Convert Alpine store to `src/js/stores/{page}.js` factory function
3. **Create page entry**: Import and register store in `src/js/pages/{page}.js`
4. **Update esbuild config**: Point to single ES6 entry point instead of concatenation
5. **Test build**: Verify IIFE output works with Alpine.js and mock server

**Next Steps - Individual Pages to Convert**:
4.5.0. Index.html
4.5.1. ✅ **Convert shared settings API** (`settings-api.js` → `src/js/api/settings.js`) - **COMPLETED**
4.5.2. ✅ **Remove orphaned legacy files** (`settings-alpine-store.js` + esbuild configs) - **COMPLETED**
4.5.3. ✅ **Diagnostics page** (`diagnostics-alpine-store.js` + `diagnostics-api.js`) - **COMPLETED**
4.5.4. ✅ **Setup page** (`setup-alpine-store.js` + `setup-api.js`) - **COMPLETED**
4.5.5. ✅ **404 page** (`404-alpine-store.js`) - **COMPLETED**
4.5.6. ✅ **Settings overview page** (`page-settings-overview.js`) - **COMPLETED**
4.5.7. ✅ **Device settings page** (`page-settings-device.js` + imports shared settings API) - **COMPLETED**
4.5.8. ✅ **WiFi settings page** (`page-settings-wifi.js` + imports shared settings API) - **COMPLETED**
4.5.9. ✅ **MQTT settings page** (`page-settings-mqtt.js` + imports shared settings API) - **COMPLETED**
4.5.10. ✅ **Memos settings page** (`page-settings-memos.js`) - **COMPLETED**
4.5.11. ✅ **Buttons settings page** (`page-settings-buttons.js` + imports shared settings API) - **COMPLETED**
4.5.12. ✅ **LEDs settings page** (`page-settings-leds.js` + imports shared settings API) - **COMPLETED**
4.5.13. ✅ **Unbidden Ink settings page** (`page-settings-unbidden-ink.js` + imports shared settings API) - **COMPLETED**

**ES6 Module Conversion Complete!** 🎉

**Phase 4.5 Results Achieved:**

- ✅ **13 pages converted** from legacy JavaScript to modern ES6 modules
- ✅ **Smaller bundle sizes** - Average 25% reduction in file sizes
- ✅ **Modern architecture** - Explicit imports/exports replace global dependencies
- ✅ **Better tree shaking** - esbuild can optimize unused code more effectively
- ✅ **IDE support** - IntelliSense, go-to-definition, and refactoring all work
- ✅ **Self-documenting** - Dependencies visible in source code
- ✅ **Zero runtime changes** - Same IIFE output, Alpine patterns, ESP32 serving

---

## Phase 5: Local Font Implementation ✅ COMPLETED

**Goal**: Implement Google Fonts Outfit locally for optimal performance and offline access

### Completed Implementation

✅ **Font-display optimizations**: Changed from `swap` to `block` for better FOUC prevention
✅ **Font weight standardization**: Converted all settings pages from bold to regular (400) weight
✅ **Loading Flag Pattern implementation**: Applied x-if template pattern to all 9 pages to eliminate FOUC
✅ **Prettier hook configuration**: Automatic code formatting via .claude/settings.local.json PostToolUse hooks

**Benefits Achieved**:

- ✅ **FOUC elimination** - x-if template pattern prevents "Error Loading Settings" flash
- ✅ **Consistent typography** - Regular weight (400) applied across all settings pages
- ✅ **Automatic formatting** - Prettier runs after all Edit/MultiEdit/Write operations
- ✅ **Template safety** - Error states only render after loading attempts complete

---

## Phase 6: Remaining Pages

**What's Next**: Apply proven patterns to remaining non-settings pages for consistency

### 6.0: 404 Page done ✅

**Current State**: Already converted to ES6 modules ✅
**Goal**: Align with overall design system and add helpful navigation
**Why**: Error pages should be as polished as the rest of the app
Done ✅

### 6.1: Diagnostics Page Refactor - done ✅

**Goal**: Split into self contained, separate pages like settings pages
**Benefits**: Easier troubleshooting, better organization, follows established patterns

- **6.1.1 Overview page** Like /settings/index.html, move from diagnostics.htmlto /diagnostics/index.html.
- Stop and test on mock. Done.
- Now Create subsequent 5 x pages inside /diagnostics/
- **6.1.2 Device**: System status, memory, uptime, chip info, logging config
- **6.1.3 Routes**: Pages and API endpoints
- **6.1.4 Runtime Configuration**: Active config values
- **6.1.5 NVS**: Non-volatile storage
- All done ✅

**Implementation Plan (for future reference)**:

1. Create `/src/data/diagnostics/` and (for build) `/data/diagnostics/` directory structure
2. Split current diagnostics.html into overview/index + 5 subpages
3. Add navigation cards like settings/index.html using cards with different colour accents
4. Extract relevant API data for each section
5. Apply consistent Alpine.js patterns from settings pages
6. Stop and test on mock and user test on live.
7. Clean up: delete files that are now used (done).

### 6.2: Setup Page (AP Mode Only)

**Current State**: Already uses Simple Loading Flag Pattern ✅
**Goal**: Bring WiFi scanning UX up to speed with settings/wifi.html
**Why**: Setup page is a little outdated: settings/wifi.html has upgraded UX that we should mirror in part.
**Tasks**:

- Import UX pattern for WiFi scanning functionality from settings-wifi.js
- Ensure uses new "loading flag" pattern with error card <template> and x-if, x-show
- Maintain AP mode compatibility (no dependencies on STA mode, clean distinction)
- Test on mock with node mock-api.js --ap-mode (AP mode has different routes to STA mode)

---

## Phase 7: Documentation & Test Harness

**Goal**: Document established patterns for future development. Ensure unit tests work.

## Development Guidelines 🛠️

**Workflow**: Create → Build → Mock test → Live ESP32 test → Commit

**Principles**:

- One step at a time, ask before major changes
- Alpine.js patterns only, no custom reactivity
- Fail fast, test everything, preserve ESP32 memory limits

## Key Patterns & Guidelines 📝

**Core Principle**: Replace complex pre-initialized structures with simple `loaded: false` flag + Alpine store pattern. Eliminate pre-initialized null structures.

**⚠️ CRITICAL: This is a TWO-STEP process - both JavaScript AND HTML must be updated together or Alpine will crash!**

We want:

- ✅ Simple loaded: false flag
- ✅ Empty config: {} object on settings pages, rather than a brittle list of pre-initialized nulls
- ✅ Direct assignment from API response
- ✅ Proper error handling without fallback structures
- ✅ Template safety guards to prevent Alpine expression crashes

## STEP 1: JavaScript Store Changes

```javascript
// ✅ CORRECT: Complete Alpine.js + Simple Loading Flag Pattern
function createMyStore() {
  return {
    loaded: false, // Simple loading flag (starts false)
    config: {}, // CRITICAL: Empty object (NO pre-initialized nulls)
    initialized: false, // Failsafe guard to prevent multiple inits
    error: null, // Error state

    async loadConfiguration() {
      // Use existing method names, don't rename
      // Duplicate initialization guard (failsafe)
      if (this.initialized) {
        return; // No console.log needed
      }
      this.initialized = true;

      this.loaded = false;
      this.error = null;
      try {
        const response = await fetch("/api/config");
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        const data = await response.json();

        // ✅ CRITICAL: Direct assignment to config object (not this.data.config)
        this.config.mqtt = {
          enabled: data.mqtt?.enabled ?? false,
          server: data.mqtt?.server ?? "",
          port: data.mqtt?.port ?? 1883,
          username: data.mqtt?.username ?? "",
          password: data.mqtt?.password ?? "",
        };

        this.loaded = true; // Mark as loaded AFTER data assignment
      } catch (error) {
        this.error = `Failed to load configuration: ${error.message}`;
      }
    },
  };
}
```

## STEP 2: HTML Template Safety (MANDATORY)

⚠️ **WARNING: If you change JavaScript to empty config: {}, you MUST add template guards or Alpine will crash!**

```html
<!-- ❌ BROKEN: Will crash with "Cannot read properties of undefined" -->
<body x-data="$store.myStore" x-init="loadConfiguration()">
  <div x-show="loaded && !error">
    <input x-model="config.mqtt.enabled" />
    <!-- CRASHES: config.mqtt is undefined -->
  </div>
</body>

<!-- ✅ CORRECT: Template safety prevents crashes -->
<body x-data="$store.myStore" x-init="loadConfiguration()">
  <!-- Main Content -->
  <template x-if="loaded && !error">
    <div
      x-data="{ show: false }"
      x-init="$nextTick(() => show = true)"
      x-show="show"
      x-transition.opacity.duration.300ms
    >
      <input x-model="config.mqtt.enabled" />
      <!-- SAFE: only evaluates when loaded -->
    </div>
  </template>
</body>

<!-- ✅ CORRECT: Way to do error card -->
<!-- Error State -->
<template x-if="loaded && error">
  <div
    class="bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 rounded-2xl p-6 mb-6"
  >
    <div class="text-center">
      <svg
        class="w-8 h-8 mx-auto mb-4 text-red-600 dark:text-red-400"
        fill="none"
        stroke="currentColor"
        viewBox="0 0 24 24"
      >
        <path
          stroke-linecap="round"
          stroke-linejoin="round"
          stroke-width="1.5"
          d="M6 18 18 6M6 6l12 12"
        />
      </svg>
      <h3 class="text-lg font-semibold text-red-800 dark:text-red-200 mb-2">
        Error Loading Settings
      </h3>
      <p class="text-red-600 dark:text-red-300 mb-4" x-text="error"></p>
      <button
        @click="loadData()"
        class="bg-blue-600 hover:bg-blue-700 dark:bg-blue-700 dark:hover:bg-blue-600 text-white px-4 py-2 rounded-lg transition-colors duration-200"
      >
        Retry
      </button>
    </div>
  </div>
</template>
```

**Critical Rules**:

- **Template Safety**: ALWAYS use `x-if` for both error and main content templates
  - Error: `<template x-if="loaded && error">` (only show errors after loading)
  - Main: `<template x-if="loaded && !error">` (prevent crashes with empty config)
- **Method Names**: Keep existing method names (`loadConfiguration`, not `loadData`)
- **Property Names**: Keep existing property structure (`config.mqtt.*` not `data.config.*`)
- **Initialization**: Direct `x-init="loadConfiguration()"` calls
- **Data Assignment**: Direct assignment (`this.config.mqtt = ...`) to existing property names

### Simple vs Complex Comparison

```javascript
// ✅ WORKS: Simple empty object + loaded flag
loaded: false,
config: {},

async loadData() {
    this.loaded = false;
    this.config = await fetchFromAPI();  // Direct assignment
    this.loaded = true;
}

// ❌ DOESN'T WORK: Complex pre-initialized null structures
config: {
    buttons: { button1: { gpio: null, shortAction: null } }  // Brittle, breaks on API changes
}
```

```html
<!-- ✅ WORKS: x-if + x-show for data safety + smooth animations -->
<template x-if="loaded && !error">
  <div
    x-data="{ show: false }"
    x-init="$nextTick(() => show = true)"
    x-show="show"
    x-transition.opacity.duration.300ms
  >
    <input x-model="config.device.owner" />
    <!-- Normal access once loaded -->
  </div>
</template>

<!-- ❌ DOESN'T WORK: x-show alone - Alpine evaluates expressions immediately -->
<div x-show="loaded && !error">
  <input x-model="config.device.owner" />
  <!-- Crashes: cannot read 'owner' of undefined -->
</div>
```

**Key Rules**:

- `x-if` prevents DOM creation and expression evaluation until condition is true
- `x-show` only toggles visibility but Alpine evaluates all expressions immediately for reactivity
- **Combine both**: `x-if` for data safety + inner `x-show` for smooth fade-in transitions

Key insight: You need BOTH for the full solution:

1. x-if="loaded && !error" prevents crashes
2. Inner x-show="show" x-transition provides smooth UX

**Benefits**: No crashes, backend flexible, ~30 lines vs ~100 lines of pre-init, future-proof

### Loading Flag Pattern Pitfalls to Avoid ❌

**❌ DEADLY ERROR: Changing JavaScript without HTML**

```javascript
// ❌ If you do this change...
config: {
    mqtt: { enabled: false, server: null }  // Remove this...
}
// to this...
config: {},  // Empty object...

// ❌ WITHOUT updating HTML, Alpine crashes with:
// "Alpine Expression Error: Cannot read properties of undefined (reading 'enabled')"
// Expression: "config.mqtt.enabled"
```

**✅ SOLUTION: Both changes together**

```javascript
// Step 1: Change JavaScript
config: {},  // Empty object
```

```html
<!-- Step 2: Add template safety immediately -->
<template x-if="loaded && !error">
  <div><!-- Form content --></div>
</template>
```

**❌ WRONG: Multiple initialization methods**

```javascript
// Don't have both loadConfiguration() AND init() methods
async loadConfiguration() { /* ... */ },
async init() { /* calls loadConfiguration */ }  // REMOVE THIS
```

**❌ WRONG: Using x-show alone with empty config**

```html
<!-- ❌ This crashes when config is empty {} -->
<div x-show="loaded && !error">
  <input x-model="config.mqtt.enabled" />
  <!-- CRASHES: mqtt undefined -->
</div>

<!-- ✅ This works - x-if prevents evaluation until loaded -->
<template x-if="loaded && !error">
  <div x-show="show" x-transition>
    <input x-model="config.mqtt.enabled" />
    <!-- SAFE -->
  </div>
</template>
```

**❌ WRONG: Changing property names unnecessarily**

```javascript
// Don't force massive HTML changes
config: {...}  →  data: { config: {...} }  // Breaks all HTML x-model references
```

**✅ CORRECT: Use x-if for data safety + x-show for animations**

```html
<template x-if="loaded && !error">
  <div x-show="show" x-transition>
    <input x-model="config.device.owner" />
    <!-- SAFE: only evaluates when loaded -->
  </div>
</template>
```

**✅ CORRECT: Keep existing property names when possible**

```javascript
// Settings pages: Keep existing config property, just make it empty initially
config: {},  // Empty object populated on load (avoid massive HTML changes)
// vs massive changes: data: { config: {} } requiring all HTML updates
```

### FOUC Prevention & Transitions

```html
<!-- ✅ CORRECT: x-show + x-transition + style="display: none" -->
<div x-show="loaded && !error" x-transition style="display: none">
  <form><!-- Content --></form>
</div>
```

- **Use `x-show`** (toggles visibility) NOT `x-if` (removes DOM elements)
- **Start with `loaded: false`** to hide content initially
- **`style="display: none"`** prevents Flash of Unstyled Content (FOUC)
