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

### Step 3.1: Create Test Page (Device)
- [ ] Create standalone `device.html` page with navigation
- [ ] Create focused `page-device.js` Alpine store
- [ ] Copy organized DEVICE CONFIGURATION API functions from main store
- [ ] Include UTILITY FUNCTIONS section (showErrorMessage)
- [ ] Test complete device functionality: owner, timezone, GPIO pins
- [ ] Verify proof of concept for page separation with organized code

### Standard Section Pattern
1. Create `[section].html` + `page-[section].js` (Alpine store with copied functions)
2. Alpine store: state + UI logic + API functions (code duplication)
3. Test section page thoroughly
4. Verify 100% functionality before next page

### Remaining Section Pages
- [ ] **WiFi**: Network scanning, SSID/password, connection timeout
- [ ] **MQTT**: Configuration, connection testing  
- [ ] **LEDs**: Effects, colors, GPIO validation
- [ ] **Memos**: Content save/load
- [ ] **Unbidden Ink**: AI configuration, scheduling
- [ ] **Buttons**: GPIO configuration, button actions
- [ ] **System**: Restart, factory reset, NVS operations

### Navigation & Cleanup
- [ ] Add client-side navigation between pages
- [ ] Create overview page with section links
- [ ] Remove monolithic files after all pages work
- [ ] Clean up unused partials

---

## Phase 4: Build System Optimization 🚀
*Resolve bundling constraints and optimize builds*

- [ ] Fix esbuild multi-entry plugin to support imports
- [ ] Enable proper module separation with external files  
- [ ] Configure code splitting per settings page
- [ ] Create page-specific CSS bundles
- [ ] Add build size monitoring and hot reload
- [ ] Eliminate code duplication through proper imports

---

## Phase 5: Future Planning 📋
*Apply lessons to remaining pages*

- [ ] Document working patterns from settings refactor
- [ ] Create new refactor plan for index, diagnostics, 404 pages
- [ ] Define templates for page structure and API patterns

---

## Implementation Strategy 🛠️

### Standard Testing Workflow
1. **Code changes** - Implement functionality
2. **Build frontend** - `npm run build-js-settings` or `npm run build`
3. **Test with mock server** - `node mock-server/mock-api.js`
4. **Verify functionality** - Test all affected features work correctly
5. **Git commit** - Clear commit message with detailed summary
6. **STOP** - Verify 100% before proceeding

### Core Principles
- **One step at a time** - Complete fully before proceeding
- **Keep old code functional** - Main settings.html works throughout
- **Alpine.js patterns only** - No hacks, use built-in reactivity
- **Fail fast** - No fallback values, let Alpine handle missing data
- **Test everything** - Every endpoint, form, validation

### Success Criteria
- All settings functionality identical to current
- Code more maintainable and modular
- No regressions in UX
- ESP32 memory usage unchanged

**Next Step:** Phase 3.1 - Create Test Page (Device)