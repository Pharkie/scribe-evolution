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
- [X] Remove `settings-old.html` (legacy monolithic file)
- [X] Clean up unused partials in `/partials/settings/`
- [X] Verify no broken internal links remain
- [X] Final end-to-end testing of complete settings workflow

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

### 4.5: ES6 Module System

**Conversion Process**: ES6 modules in source → IIFE bundles for ESP32

**Per-Page Conversion Steps**:
1. **Create API module**: Move API functions to `src/js/api/{page}.js` with ES6 exports
2. **Create store module**: Convert Alpine store to `src/js/stores/{page}.js` factory function  
3. **Create page entry**: Import and register store in `src/js/pages/{page}.js`
4. **Update esbuild config**: Point to single ES6 entry point instead of concatenation
5. **Test build**: Verify IIFE output works with Alpine.js and mock server

**Completed Pages**: 
- **✅ Index page** - Full ES6 conversion completed
  - `src/js/api/index.js`: 6 exported API functions
  - `src/js/stores/index.js`: `createIndexStore()` factory with explicit imports
  - `src/js/pages/index.js`: Alpine registration entry point  
  - Build output: `data/js/page-index.js` (28KB dev, 14KB prod)

**Benefits Realized**:
- **Explicit dependencies**: Clear `import` statements replace mysterious globals
- **Superior IDE support**: IntelliSense, go-to-definition, refactoring all work
- **Self-documenting architecture**: Dependencies visible in source code
- **Zero runtime changes**: Same IIFE output, Alpine patterns, ESP32 serving

**Next Steps - Individual Pages to Convert**:
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

**Build Size Improvements:**
- WiFi: 21,397 → 17,481 bytes (**18% smaller**)
- MQTT: 17,769 → 12,033 bytes (**32% smaller**)
- Buttons: 16,377 → 10,224 bytes (**38% smaller**)
- LEDs: 15,758 → 11,212 bytes (**29% smaller**)
- Unbidden Ink: 17,402 → 12,069 bytes (**31% smaller**)

Each page follows same 5-step conversion process above

---

## Phase 5: Remaining Pages

Apply proven settings patterns to remaining pages

- **5.0 Setup.html (only accessible in AP mode)** 
  - Already uses "the Alpine Load Flag process". 
  - But bring up to speed with UX in settings > wifi.html
- **5.1 Index Page** - Extract to `page-index.js`, modular architecture
- **5.2 Diagnostics** - Split sections into proper pages like settings (Overview + Microcontroller, Logging, Routes, Config, NVS)
- **5.3 404 Page** - Polish and align with architecture
- **5.4 Documentation** - Pattern templates and coding standards

---

## Phase 6: Font Review ✨

Typography improvements and font weight optimization across the application.

**Status**:

Review fonts in terms of alternatives and ligatures. Check why headings still use bold instead of normal, 400 weight.

### 6.1: Google Fonts Outfit Integration - ✅ COMPLETED
- **Issue discovered**: Tailwind v4 build system not reading `tailwind.config.js`
- **Root cause**: v4 uses CSS-first configuration, not legacy config file approach
- **Solution implemented**: Proper Tailwind v4 CSS-first configuration using `@theme` directive
- **Build system fixed**: Removed obsolete config file, updated to modern v4 methodology

### 6.2: Font Weight Optimization - ✅ COMPLETED  
- **Problem**: Heavy `font-bold` (700-900 weight) inappropriate for Outfit font
- **Solution**: Adjusted key headings to `font-normal` (400 weight) for better hierarchy
- **Components updated**: Settings headers, page titles, diagnostic titles, status headers
- **Result**: More elegant, readable typography with proper visual balance

---

## Phase 7: AI Memos 🤖

Future enhancement: 4 x configurable AI prompts with hardware button integration

---

## Development Guidelines 🛠️

**Workflow**: Create → Build → Mock test → Live ESP32 test → Commit

**Principles**: 
- One step at a time, ask before major changes
- Alpine.js patterns only, no custom reactivity
- Fail fast, test everything, preserve ESP32 memory limits

## Key Patterns & Guidelines 📝

### Simple Loading Flag Pattern with Alpine.js
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
        loaded: false,  // Simple loading flag (starts false)
        config: {},     // CRITICAL: Empty object (NO pre-initialized nulls)
        initialized: false, // Failsafe guard to prevent multiple inits
        error: null,    // Error state
        
        async loadConfiguration() {  // Use existing method names, don't rename
            // Duplicate initialization guard (failsafe)
            if (this.initialized) {
                return;  // No console.log needed
            }
            this.initialized = true;
            
            this.loaded = false;
            this.error = null;
            try {
                const response = await fetch('/api/config');
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}: ${response.statusText}`);
                }
                const data = await response.json();
                
                // ✅ CRITICAL: Direct assignment to config object (not this.data.config)
                this.config.mqtt = {
                    enabled: data.mqtt?.enabled ?? false,
                    server: data.mqtt?.server ?? '',
                    port: data.mqtt?.port ?? 1883,
                    username: data.mqtt?.username ?? '',
                    password: data.mqtt?.password ?? ''
                };
                
                this.loaded = true;  // Mark as loaded AFTER data assignment
            } catch (error) {
                this.error = `Failed to load configuration: ${error.message}`;
            }
        }
    };
}
```

## STEP 2: HTML Template Safety (MANDATORY)

⚠️ **WARNING: If you change JavaScript to empty config: {}, you MUST add template guards or Alpine will crash!**

```html
<!-- ❌ BROKEN: Will crash with "Cannot read properties of undefined" -->
<body x-data="$store.myStore" x-init="loadConfiguration()">
    <div x-show="loaded && !error">
        <input x-model="config.mqtt.enabled"> <!-- CRASHES: config.mqtt is undefined -->
    </div>
</body>

<!-- ✅ CORRECT: Template safety prevents crashes -->
<body x-data="$store.myStore" x-init="loadConfiguration()">
    <template x-if="loaded && !error">
        <div x-data="{ show: false }" x-init="$nextTick(() => show = true)" 
             x-show="show" x-transition.opacity.duration.300ms>
            <input x-model="config.mqtt.enabled"> <!-- SAFE: only evaluates when loaded -->
        </div>
    </template>
</body>
```

**Critical Rules**:
- **Template Safety**: ALWAYS use `x-if="loaded && !error"` when config starts empty
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
    <div x-data="{ show: false }" x-init="$nextTick(() => show = true)" 
         x-show="show" x-transition.opacity.duration.300ms>
        <input x-model="config.device.owner"> <!-- Normal access once loaded -->
    </div>
</template>

<!-- ❌ DOESN'T WORK: x-show alone - Alpine evaluates expressions immediately -->
<div x-show="loaded && !error">
    <input x-model="config.device.owner"> <!-- Crashes: cannot read 'owner' of undefined -->
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
    <input x-model="config.mqtt.enabled"> <!-- CRASHES: mqtt undefined -->
</div>

<!-- ✅ This works - x-if prevents evaluation until loaded -->
<template x-if="loaded && !error">
    <div x-show="show" x-transition>
        <input x-model="config.mqtt.enabled"> <!-- SAFE -->
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
        <input x-model="config.device.owner"> <!-- SAFE: only evaluates when loaded -->
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

### Development Essentials
- **Partial Config Updates**: Only send relevant sections, not entire config
- **Mock Server First**: Test before live ESP32 verification  
- **Pattern Consistency**: Copy existing page structures, don't reinvent
- **Error Handling**: Fail fast, let Alpine handle missing data
- **Console Cleanup**: Remove verbose API messages, keep concise emoji-prefixed messages for debugging

---

## Rejected Ideas 🚫

**Partial Config API Endpoints**: Separate endpoints like `/api/config/device` rejected due to ESP32 memory constraints and over-engineering. Current `/api/config` works fine with client-side filtering.

