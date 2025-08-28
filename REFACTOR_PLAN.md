# Settings Refactor Plan 🎯

**Goal:** Modernize and modularize the settings system without breaking functionality.

**Current State:**
- 1704-line monolithic `settings-alpine-store.js` 
- HTML partials system working well
- Terser-based build system functional but slow
- All functionality working (known-good state)

## Phase 1: Build System Modernization ⚡

### Step 1.1: Install and Configure esbuild
- [ ] Install esbuild as dev dependency
- [ ] Create `esbuild.config.js` with current terser equivalent
- [ ] Test esbuild builds match current output sizes/functionality  
- [ ] Update one page build script as proof of concept
- [ ] Verify all pages still work identically

### Step 1.2: Migrate Build Scripts
- [ ] Replace all terser commands with esbuild equivalents
- [ ] Update package.json build scripts
- [ ] Test parallel builds and watch mode
- [ ] Verify build performance improvements
- [ ] Test that all minified outputs work identically

**Expected Outcome:** Faster builds, same functionality
**Safety:** Can revert build scripts if anything breaks

---

## Phase 2: JavaScript Architecture Modularization 🏗️

### Step 2.1: Extract Settings API Layer  
- [ ] Create `src/js/settings/api/` directory structure:
  - `device-api.js` - device/AP/WiFi API calls
  - `mqtt-api.js` - MQTT testing and config  
  - `led-api.js` - LED effects and config
  - `system-api.js` - restart, factory reset, NVS operations
- [ ] Move API functions from `settings-alpine-store.js` to dedicated files
- [ ] Update imports and test all API functionality works
- [ ] Verify settings save/load still works perfectly

### Step 2.2: Extract Utility Functions
- [ ] Create `src/js/settings/utils/` directory:
  - `gpio-utils.js` - GPIO validation, pin descriptions  
  - `time-utils.js` - timezone, time formatting functions
  - `validation-utils.js` - form validation logic
  - `color-utils.js` - LED color handling
- [ ] Move utility functions from main store
- [ ] Test all form validations work
- [ ] Verify LED color picker functionality

### Step 2.3: Break Store into Logical Sections  
- [ ] Create focused Alpine stores:
  - `settings-device-store.js` - WiFi, AP mode, device config
  - `settings-mqtt-store.js` - MQTT configuration and testing
  - `settings-led-store.js` - LED effects and configuration  
  - `settings-system-store.js` - buttons, memos, unbidden ink
- [ ] Keep main `settings-alpine-store.js` as coordinator
- [ ] Implement proper inter-store communication
- [ ] Test each section works independently

**Expected Outcome:** More maintainable JS, same UX
**Safety:** Keep old file as backup until fully verified

---

## Phase 3: HTML Structure Evolution 📄

### Step 3.1: Enhanced Partials (Current Structure++)
- [ ] Add loading states to all partials
- [ ] Implement proper error handling per section  
- [ ] Add section-level validation feedback
- [ ] Enhance mobile responsiveness
- [ ] Test all partials load and function correctly

### Step 3.2: Independent Section Pages (Future)
**ONLY IF Phase 2 is completely stable:**
- [ ] Create `/settings/device`, `/settings/mqtt` etc. routes  
- [ ] Convert partials to full pages with navigation
- [ ] Add breadcrumb navigation system
- [ ] Implement state preservation between sections

**Expected Outcome:** Better UX, cleaner URLs
**Safety:** Keep partials system as fallback

---

## Phase 4: Build System Optimization 🚀

### Step 4.1: Bundle Optimization
- [ ] Implement code splitting per page
- [ ] Tree shake unused Alpine.js features
- [ ] Optimize CSS bundle sizes per page
- [ ] Add build size monitoring

### Step 4.2: Development Experience  
- [ ] Hot reload for CSS/JS changes
- [ ] Better source maps for debugging
- [ ] Lint integration with esbuild
- [ ] Bundle analysis tools

---

## Implementation Strategy 🛠️

### Safety First Principles:
1. **One phase at a time** - Don't start next phase until current is 100% stable
2. **Keep old code** - Don't delete anything until replacement is proven
3. **Test everything** - Every API endpoint, every form, every validation
4. **Commit early** - Small, focused commits for easy rollback
5. **User testing** - Verify all settings save/load correctly on real hardware

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

**Next Step:** Start with Phase 1.1 - esbuild installation and testing