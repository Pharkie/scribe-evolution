# App Refactor Plan 🎯

**Goal:** Modernize and modularize the settings system without breaking functionality.

**Current Status**: Phase 3 complete (3.1-3.8), ready for step 3.9 Legacy Cleanup

## ✅ COMPLETED PHASES 1-2

### Phase 1: Build System Modernization ⚡
- [x] esbuild integration (3x faster builds)
- [x] Dev/prod file naming (`.js`/`.min.js`)
- [x] Mock server support

### Phase 2: Internal Organization 🏗️  
- [x] Section separation (Device/WiFi partials)
- [x] API function organization within main store
- [x] Utility structure preparation

---

## Phase 3: Complete Page Architecture ✅

**Status**: All individual settings pages complete, ready for cleanup

### Completed Settings Pages (3.1-3.8)
- **Device** (blue) - Owner, timezone, GPIO pins, data-driven config system
- **WiFi** (green) - Network credentials, connection status  
- **MQTT** (red) - Server config, connection testing, enable/disable
- **Memo** (orange) - Content editor, placeholders, character counter
- **Button** (cyan) - Hardware actions, short/long press, MQTT integration  
- **LED** (pink) - Effects config, playground testing, C++ alignment
- **Unbidden Ink** (purple) - AI prompts, scheduling, API settings

**Testing Status**: Steps 3.1-3.8 need live ESP32 verification before proceeding to 3.9

**Key Features Delivered**:
- Color-coded themes per section
- Partial config updates (send only relevant fields)
- Consistent Alpine.js patterns and error handling
- Mock server testing + live ESP32 verification
- Navigation hierarchy: Home → Settings → Individual Pages

### Step 3.8: Unbidden Ink Settings [NEEDS LIVE TEST]
- [x] Purple theme, AI content configuration
- [x] Prompts, scheduling, API settings
- [ ] Live ESP32 testing pending

### Step 3.9: Legacy Cleanup [READY]
- [ ] Remove `settings-old.html` (legacy monolithic file)
- [ ] Clean up unused partials in `/partials/settings/`
- [ ] Verify no broken internal links remain
- [ ] Final end-to-end testing of complete settings workflow

---

## Phase 4: Optimization 🚀

### Completed
- **REST API Standardization**  - Success: HTTP 200 + empty body, Error: Non-200 + JSON
- **Partial Config Pattern**  - Settings pages send only relevant sections
- PATCH and PUT instead of just GET and POST?

### Remaining  
- **4.1 Module System** - Fix esbuild imports, enable code splitting
- **4.2 CSS Optimization** - Address 60-80KB Tailwind files (consider gzip/shared builds)
- **4.3 Resource Optimization** - Large files: `timezones.json` (174KB), `riddles.ndjson` (111KB)

---

## Phase 5: Remaining Pages

Apply proven settings patterns to remaining pages

- **5.1 Index Page** - Extract to `page-index.js`, modular architecture
- **5.2 Diagnostics** - Split sections into proper pages like settings (Overview + Microcontroller, Logging, Routes, Config, NVS)
- **5.3 404 Page** - Polish and align with architecture
- **5.4 Documentation** - Pattern templates and coding standards

---

## Phase 6: AI Memos 🤖

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
**Core Principle**: Replace complex pre-initialized structures with simple `loaded: false` flag + Alpine store pattern

**Critical Timing Rule**: Store initialization MUST happen AFTER Alpine establishes DOM binding

```javascript
// ✅ CORRECT: Complete Alpine.js + Simple Loading Flag Pattern
function createMyStore() {
    return {
        loaded: false,  // Simple loading flag (starts false)
        config: {},       // Empty data or config (settings pages) object (populated on load)
        initialized: false, // Failsafe guard to prevent multiple inits
        error: null,    // Error state
        
        async loadData() {
            // Duplicate initialization guard (failsafe)
            if (this.initialized) {
                console.log('Already initialized, skipping');
                return;
            }
            this.initialized = true;
            
            this.loaded = false;
            this.error = null;
            try {
                this.data.config = await fetchFromAPI();  // Direct assignment
                this.loaded = true;
            } catch (error) {
                this.error = error.message;
            }
        }
    };
}

document.addEventListener('alpine:init', () => {
    Alpine.store('myStore', createMyStore());
});
```

```html
<!-- HTML: Complete pattern implementation -->
<body x-data="$store.myStore" x-init="loadData()">
    <template x-if="loaded && !error">
        <div x-data="{ show: false }" x-init="$nextTick(() => show = true)" 
             x-show="show" x-transition.opacity.duration.300ms>
            <input x-model="data.config.deviceName"> <!-- SAFE: only evaluates when loaded -->
        </div>
    </template>
</body>
```

**Critical Rules**:
- **Method Naming**: Use consistent `loadData()` method name across all pages
- **Store Pattern**: Always use `$store.storeName` not direct instances  
- **Single Method**: Only ONE initialization method per store (avoid duplicate `init()` methods)
- **Template Safety**: Use `x-if="loaded && !error"` to prevent expression evaluation before data loads
- **Initialization**: Direct `x-init="loadData()"` calls (no `$nextTick` needed for loadData)
- **Data Assignment**: Direct assignment (`this.data.config = serverData`) not pre-initialized objects

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

**❌ WRONG: Multiple initialization methods**
```javascript
// Don't have both loadData() AND init() methods - causes duplicate calls
async loadData() { /* ... */ },
async init() { /* calls loadData or duplicates logic */ }  // REMOVE THIS
```

**❌ WRONG: Direct store instances**  
```html
<!-- Don't use direct instances -->
<body x-data="window.myStoreInstance">
```
```javascript
window.myStoreInstance = createStore(); // Avoid this pattern
```

**❌ WRONG: Using x-show without x-if for async data**
```html
<!-- Don't do this - Alpine evaluates expressions immediately -->
<div x-show="loaded && !error">
    <input x-model="config.device.owner"> <!-- CRASHES: config undefined -->
</div>
```

**❌ WRONG: Massive pointless property renaming**
```javascript
// Don't rename existing working properties if easily avoided
// BAD: Changing all HTML references from config.* to data.config.*
config: { device: {...} }  →  data: { config: { device: {...} } }  // AVOID THIS
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

---

## Rejected Ideas 🚫

**Partial Config API Endpoints**: Separate endpoints like `/api/config/device` rejected due to ESP32 memory constraints and over-engineering. Current `/api/config` works fine with client-side filtering.

