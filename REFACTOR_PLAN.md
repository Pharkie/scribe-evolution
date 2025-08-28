# Settings Refactor Plan 🎯

**Goal:** Modernize and modularize the settings system without breaking functionality.

**Current State:**
- 1704-line monolithic `settings-alpine-store.js` 
- HTML partials system working well
- ✅ **esbuild-based build system fast and optimized**
- All functionality working (known-good state)

## Phase 1: Build System Modernization ⚡ ✅ COMPLETED

### Step 1.1: Install and Configure esbuild ✅
- [x] Install esbuild as dev dependency
- [x] Create `esbuild.config.js` with proper bundling (not just concat)
- [x] Test esbuild builds - **IMPROVED** output sizes with tree-shaking  
- [x] Verify all pages work with proper module bundling
- [x] Confirm Alpine.js functionality preserved

**Results:**
- Build time: <0.9 seconds (vs ~3+ seconds with terser)
- page-settings.min.js: 66,723 bytes (properly bundled vs 34,872 concat)
- All Alpine.js stores working perfectly
- Proper tree-shaking and optimization active
- Mock-server verified working

### Step 1.2: Migrate Build Scripts ✅ COMPLETED
- [x] Replace all terser commands with esbuild equivalents in package.json
- [x] Update npm scripts: `build`, `build-js-*`, `watch`, etc.  
- [x] Test parallel builds and watch mode - **WORKING**
- [x] Verify build performance improvements - **3x faster builds**
- [x] Test mock-server compatibility - **VERIFIED**
- [x] Remove terser dependency ✅

### Step 1.3: Build System Polish ✅ COMPLETED  
- [x] Fix file naming convention: dev builds use `.js`, production builds use `.min.js`
- [x] Rename vendor.min.js → alpine.min.js for clarity
- [x] Configure mock-server to map `.min.js` requests to readable `.js` files for debugging
- [x] Update all HTML templates to reference alpine.min.js
- [x] Verify both dev and prod builds work correctly

**Final Results:** 
- Build system fully modernized and polished
- Clear dev/prod distinction with proper file naming
- Enhanced debugging experience via mock-server mapping
- 3x faster builds with better optimization than terser
- All functionality preserved and verified

**Status:** Phase 1 100% complete, ready for Phase 2

---

## Phase 2: Foundation Extraction 🏗️
*Prepare for page separation by extracting reusable components*

### Step 2.0: Separate WiFi Section in Current Settings
- [ ] Split Device section in current `settings.html` into:
  - **Device section** - Owner, timezone, GPIO pins (6 dropdowns), basic device config
  - **WiFi section** - WiFi networks, AP mode, network configuration  
- [ ] Update current Alpine store to handle separated sections
- [ ] Test that both sections work in current monolithic settings page
- [ ] **Build frontend:** `npm run build-js-settings` - update dev JS for mock server
- [ ] **Test with mock server:** `node mock-server/mock-api.js` - verify WiFi section appears and functions
- [ ] **Git commit:** "Separate WiFi into distinct section in current settings"

### Step 2.1: Extract Settings API Layer
- [ ] Create `src/js/settings/utils/` directory structure:
  - `http-utils.js` - HTTP utility functions (error handling, request wrappers)
  - `system-utils.js` - System utility functions (GPIO validation, time formatting, etc.)
- [ ] Create section-specific **pure HTTP function** modules:
  - `settings-device-api.js` - device owner, timezone HTTP calls (no state, no Alpine)
  - `settings-wifi-api.js` - WiFi networks, AP mode, network HTTP calls
  - `settings-mqtt-api.js` - MQTT testing and config HTTP calls
  - `settings-led-api.js` - LED effects and config HTTP calls  
  - `settings-memos-api.js` - memo content save/load HTTP calls
  - `settings-unbidden-ink-api.js` - AI content configuration HTTP calls
  - `settings-buttons-api.js` - button configuration HTTP calls
  - `settings-system-api.js` - restart, factory reset, NVS HTTP calls
- [ ] Extract API functions from monolithic store, keeping them **stateless**
- [ ] Test all API functionality works independently (can be unit tested)
- [ ] **Apply Standard Testing Workflow** ⬆️ (build → test → commit → verify)

### Step 2.2: Test Extracted Components
- [ ] Verify all utils work independently:
  - HTTP request patterns from `utils/http-utils.js`
  - System utilities from `utils/system-utils.js` (GPIO, time, validation, colors)
- [ ] Test all section APIs import and use utils correctly
- [ ] Ensure no circular dependencies or missing imports
- [ ] **Apply Standard Testing Workflow** ⬆️ (build → test → commit → verify)

### Step 2.3: Create ONE Test Section Page  
- [ ] **CRITICAL:** Start with Device section (simplest, most fundamental)
- [ ] Create `device.html` with navigation back to main settings
- [ ] Create focused `page-device.js` Alpine store with:
  - **State only** (config, loading, error states)
  - **UI logic only** (computed properties, form handlers)
  - **Import and use** extracted API modules from 2.1
  - **PURE ALPINE PATTERNS** - No script tags, no hacks, proper stores/reactivity
- [ ] Alpine store calls API functions but handles **no HTTP directly**
- [ ] Test EVERYTHING: save, load, validation, navigation, error handling
- [ ] **Apply Standard Testing Workflow** ⬆️ (build → test → commit → verify)
- [ ] **STOP:** Verify this ONE page works 100% before proceeding

**Expected Outcome:** Proof of concept for page separation architecture
**Safety:** Main settings.html remains fully functional

---

## Phase 3: Complete Page Architecture 📄
*Only proceed if Phase 2.3 test page works perfectly*

### Standard Section Page Pattern 📋
**Each section follows this template:**
1. Create `[section].html` + `page-[section].js` (Alpine store + imports API modules)
2. Alpine store: **state + UI logic only**, **no direct HTTP calls**
3. Test section page thoroughly: [section-specific functionality]
4. **Apply Standard Testing Workflow** ⬆️ (build → test → commit → verify)
5. **STOP:** Verify section page works 100% before proceeding

### Step 3.1: WiFi Section Page
- [ ] **Apply Standard Section Page Pattern** ⬆️
- [ ] **Focus:** WiFi scanning, connection, AP mode

### Step 3.2: MQTT Section Page  
- [ ] **Apply Standard Section Page Pattern** ⬆️
- [ ] **Focus:** MQTT configuration, connection testing

### Step 3.3: LEDs Section Page
- [ ] **Apply Standard Section Page Pattern** ⬆️
- [ ] **Focus:** LED effects, colors, GPIO validation

### Step 3.4: Memos Section Page
- [ ] **Apply Standard Section Page Pattern** ⬆️
- [ ] **Focus:** Memo content save/load

### Step 3.5: Unbidden Ink Section Page
- [ ] **Apply Standard Section Page Pattern** ⬆️
- [ ] **Focus:** AI configuration, scheduling

### Step 3.6: Buttons Section Page
- [ ] **Apply Standard Section Page Pattern** ⬆️
- [ ] **Focus:** GPIO configuration, button actions

### Step 3.7: System Section Page
- [ ] **Apply Standard Section Page Pattern** ⬆️
- [ ] **Focus:** Restart, factory reset, NVS operations

### Step 3.8: Add Client-Side Navigation
- [ ] Create `src/js/shared/navigation.js` - Handle routing between settings pages
- [ ] Add breadcrumb navigation to all pages
- [ ] Implement state preservation between page transitions
- [ ] Add "Overview" main settings page with section links
- [ ] **Apply Standard Testing Workflow** ⬆️ (build → test → commit → verify)

### Step 3.9: Remove Monolithic Files (FINAL STEP)
- [ ] **ONLY after all pages work perfectly:**
- [ ] Remove old settings-alpine-store.js monolith
- [ ] Update main settings.html to redirect to overview page
- [ ] Clean up unused partials if desired
- [ ] **Apply Standard Testing Workflow** ⬆️ (final production build → test → commit)

---

## Phase 4: Build System Optimization 🚀
*Only proceed after Phase 3 is complete and stable*

### Step 4.1: Bundle Optimization per Page
- [ ] Configure esbuild for code splitting per settings page
- [ ] Create page-specific CSS bundles (device.css, mqtt.css, etc.)
- [ ] Optimize shared chunks (common Alpine.js, utilities)
- [ ] Add build size monitoring and reporting
- [ ] **Git commit:** "Optimize bundle sizes with page-specific builds"

### Step 4.2: Development Experience Enhancement
- [ ] Implement hot reload for CSS/JS changes during development
- [ ] Enhanced source maps for better debugging
- [ ] Integrate linting with esbuild pipeline
- [ ] Add bundle analysis tools and size tracking
- [ ] **Git commit:** "Enhance development experience and tooling"

---

## Phase 5: Future Refactor Planning 📋
*Create new refactor plan based on lessons learned from settings*

### Step 5.1: Document Lessons Learned
- [ ] Analyze what worked well in settings refactor (API separation, Alpine patterns, etc.)
- [ ] Document anti-patterns to avoid (monolithic stores, mixed concerns)
- [ ] Create reusable templates for page structure and API patterns
- [ ] **Git commit:** "Document settings refactor lessons and patterns"

### Step 5.2: Plan Remaining Pages Refactor
- [ ] Create comprehensive refactor plan for:
  - **index.html** + current monolithic structure
  - **diagnostics.html** + diagnostic systems
  - **404.html** + error handling patterns
- [ ] Apply lessons learned from settings refactor
- [ ] Define same safety principles: one step at a time, git commits, Alpine best practices
- [ ] Plan API extraction patterns for each page type
- [ ] **Deliverable:** New `REFACTOR_PLAN_PHASE2.md` for remaining pages

---

## Implementation Strategy 🛠️

### Standard Testing Workflow 🔄
**Every step follows this pattern:**
1. **Code changes** - Implement the specific functionality  
2. **Build frontend** - `npm run build-js-settings` (for settings changes) or `npm run build` (for new pages)
3. **Test with mock server** - `node mock-server/mock-api.js` - verify changes work correctly
4. **Git commit** - Clear commit message describing the change
5. **STOP** - Verify 100% functionality before proceeding to next step

### Safety First Principles:
1. **One step at a time** - Complete each step fully before proceeding
2. **Test ONE page thoroughly** - Phase 2.3 creates single test page, verify 100% before continuing
3. **Keep old code** - Main settings.html stays functional throughout Phase 2 & 3.1
4. **Git commit between phases** - Clear rollback points at every major step
5. **Test everything** - Every API endpoint, every form, every validation on each page
6. **Real hardware validation** - Verify all settings save/load correctly on ESP32

### CRITICAL: Alpine.js Best Practices ONLY ⚡
7. **ALPINE PATTERNS MUST BE RESPECTED** - No hacks, no script tags in HTML, no fucking about
8. **NO NON-STANDARD IDEAS** - If it's not Alpine best practice, ASK FIRST
9. **USE ALPINE PROPERLY** - Built-in reactivity ($watch, x-effect), stores, getters
10. **NO CUSTOM SOLUTIONS** - Don't create homebrew caching, watching, or reactivity systems
11. **FAIL FAST PRINCIPLE** - No fallback values or defensive arrays, let Alpine handle missing data

### Success Criteria:
- ✅ All settings functionality identical to current
- ✅ Build times significantly improved  
- ✅ Code is more maintainable and modular
- ✅ No regressions in mobile or desktop UX
- ✅ ESP32 memory usage unchanged or improved

### Rollback Plan:
- Each phase has a git tag for easy revert
- Keep `settings-alpine-store.js` backup until Phase 2 complete
- Build scripts can fall back to terser if needed

---

## Why This Plan Works ✅

1. **Incremental:** Each step adds value without breaking current functionality
2. **Testable:** Easy to verify each change works before proceeding  
3. **Reversible:** Clear rollback points if anything goes wrong
4. **Focused:** Each phase has a single concern (build, JS, HTML, optimization)
5. **Proven:** Based on failed attempt analysis - avoids previous pitfalls

**Next Step:** Phase 2.1 - Extract Settings API Layer (prepare foundation for settings page separation)

**Future Vision:** After completing settings refactor (Phases 1-4), Phase 5 will create a new comprehensive refactor plan for index, diagnostics, and 404 pages based on lessons learned and proven patterns.