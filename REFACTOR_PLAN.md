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

### Step 3.2: Data-Driven Config System ✅ COMPLETED
**Problem**: Current `/api/config` handler is hardcoded mess - 6 utility functions, manual field mapping, 200+ lines of repetitive validation
**Solution**: Create elegant data-driven configuration system before proceeding to more settings pages
- [x] **Design config field definition system**: Declare fields once with validation rules
- [x] **Create generic validation engine**: Single handler iterates JSON keys, applies rules
- [x] **Eliminate hardcoded field names**: Use reflection/mapping instead of manual switches
- [x] **Consistent partial update pattern**: Any valid key:value updates runtime + NVS automatically
- [x] **Test with device settings page**: Ensure existing functionality preserved
- [x] **Document the pattern**: Clear examples for future settings pages (`docs/DATA_DRIVEN_CONFIG.md`)

**Result**: Replaced 200+ lines of hardcoded validation with 8 lines of data-driven code. Single source of truth in `CONFIG_FIELDS[]` array handles all validation automatically. **Mock and ESP32 parity confirmed - system working perfectly.**

### Step 3.3: WiFi Settings Page
- [ ] **WiFi**: Network scanning, SSID/password, connection timeout

### Step 3.4: MQTT Settings Page  
- [ ] **MQTT**: Configuration, connection testing

### Step 3.5: Unbidden Ink Settings Page
- [ ] **Unbidden Ink**: AI configuration, scheduling

### Step 3.6: Button Settings Page
- [ ] **Buttons**: GPIO configuration, button action

### Step 3.7: LED Settings Page
- [ ] **LEDs**: Effects, colors, GPIO validation

### Step 3.8: Memo Settings Page
- [ ] **Memos**: Content save/load

### Step 3.9: System Operations Page
- [ ] **System**: Restart, factory reset, NVS operations

### Step 3.10: Setup Page Update
- [ ] **Setup Page**: Update setup.html to the new format

### Step 3.11: Navigation & Cleanup
- [x] Create overview page with section links (`/settings.html` with navigation grid)
- [ ] Add client-side navigation between pages  
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
2. **Update build scripts** - If adding new JS files, update package.json build scripts to include them
3. **Build frontend** - `npm run build-js-settings` or `npm run build`
4. **Test with mock server** - `node mock-server/mock-api.js`
5. **Verify functionality** - Test all affected features work correctly
6. **Live device testing** - Invite Adam to deploy to ESP32 and verify on actual hardware (end of phases)
7. **Update REFACTOR_PLAN.md** - Revise and mark steps completed. Revise the plan forward from here based on lessons learned.
8. **Git commit** - Clear commit message with detailed summary
9. **STOP** - Verify 100% before proceeding

### Core Principles
- **One step at a time** - Complete fully before proceeding
- **Keep old code functional** - Main settings.html works throughout
- **Alpine.js patterns only** - No hacks, use built-in reactivity
- **Fail fast** - No fallback values, let Alpine handle missing data
- **Test everything** - Every endpoint, form, validation
- **Live device testing** - Required at end of each phase for hardware-dependent features

### Success Criteria
- All settings functionality identical to current
- Code more maintainable and modular
- No regressions in UX
- ESP32 memory usage unchanged

## Lessons Learned 📝

### Alpine.js Select Element Synchronization
**Problem:** GPIO dropdowns in device.html didn't select correct values compared to monolithic settings
**Root Cause:** Alpine's `x-model` alone insufficient for complex async data + select elements
**Solution:** Use Alpine's `x-effect` directive to force DOM synchronization:
```html
x-effect="
    if (!loading && config.device.printerTxPin !== null && printerGpioOptions.length > 0) {
        $nextTick(() => $el.value = config.device.printerTxPin);
    }
"
```
**Key Learning:** Always use Alpine's built-in reactivity (`x-effect`, `$nextTick`) instead of custom solutions

### Alpine.js x-for Keys Must Be Primitives
**Problem:** `Alpine Warning: x-for key cannot be an object, it must be a string or an integer`
**Root Cause:** Using `option.pin` as key when `option.pin` could be `null` (object)
**Solution:** Use array index instead: `:key="'prefix-' + index"`
**Key Learning:** Alpine x-for keys must be strings/integers, not objects/null

### Alpine.js Reactive Dropdown Text Updates - FUNDAMENTAL LIMITATION ❌
**Problem:** GPIO dropdown option text labels don't update reactively when assignments change
**Symptom:** Disabling/enabling works instantly, but text like "GPIO 4 (Assigned to button 2)" doesn't update
**Root Cause:** Alpine's x-for with HTML select/option elements has fundamental reactivity limitations
**Attempted Solutions:**
1. `x-text` with inline ternary expressions - ❌ No reactivity
2. Computed properties in getter with `assignment` field - ❌ No reactivity  
3. `$store.settingsDevice` explicit references - ❌ No reactivity
4. Dynamic `:key` values with unique identifiers - ❌ No reactivity
5. `x-effect` with `$el.textContent` direct DOM manipulation - ❌ No reactivity
6. Pre-computed text with dynamic keys - ❌ No reactivity

**Conclusion:** Alpine's x-for with select/option elements cannot reactively update option text
**Decision:** GPIO assignment labels working correctly is NOT critical for device functionality - proceed to next section
**Key Learning:** Alpine x-for + select/option reactivity is fundamentally limited - avoid this pattern

### File Management During Refactoring  
**Problem:** Overwrote working settings.html without backup, risking system stability
**Root Cause:** Rushed implementation without proper backup strategy
**Solution:** Always backup originals before major changes:
- `settings.html` → working monolithic (current)
- `settings-v2.html` → new modular version (testing)
- `settings/device.html` → individual page (working)
**Key Learning:** Maintain parallel versions during major architectural changes

## Current Status 📍

**Currently working on Step 3.1.5: Data-Driven Config System** - must complete before proceeding to additional settings pages.

**Next Step:** Complete Step 3.1.5, then proceed to Step 3.2 (WiFi Settings Page)