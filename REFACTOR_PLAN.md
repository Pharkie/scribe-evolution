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

**⚠️ CRITICAL BUILD NOTE:**
- Mock server uses **DEV assets** (`.js` files) not production (`.min.js`)
- Always build dev versions for local testing: `npm run build-js-overview` NOT `npm run build-js-overview-prod`
- Production builds are for ESP32 deployment only

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

### Step 3.2: Data-Driven Config System: Device Page
**Problem**: Current `/api/config` handler is hardcoded mess - 6 utility functions, manual field mapping, 200+ lines of repetitive validation
**Solution**: Create elegant data-driven configuration system before proceeding to more settings pages
- [x] **Design config field definition system**: Declare fields once with validation rules
- [x] **Create generic validation engine**: Single handler iterates JSON keys, applies rules
- [x] **Eliminate hardcoded field names**: Use reflection/mapping instead of manual switches
- [x] **Consistent partial update pattern**: Any valid key:value updates runtime + NVS automatically
- [x] **Test with device settings page**: Ensure existing functionality preserved
- [x] **Document the pattern**: Clear examples for future settings pages (`docs/DATA_DRIVEN_CONFIG.md`)

### Step 3.3: WiFi Settings Page
**Problem**: WiFi configuration exists in monolithic settings.html with mixed concerns - single file contains all settings sections and JavaScript  
**Solution**: Extract existing WiFi interface (text fields, layout, functionality) from settings-old.html into dedicated `/settings/wifi.html` with separate `page-settings-wifi.js` Alpine store to enable eventual elimination of monolithic settings.html

- [ ] Run Testing Workflow

### Step 3.4: MQTT Settings Page **← NEXT STEP**
**Problem**: MQTT configuration exists in monolithic settings.html with mixed concerns - single file contains all settings sections and JavaScript
**Solution**: Extract existing MQTT interface (server, port, credentials, enable toggle) from settings-old.html into dedicated `/settings/mqtt.html` with separate `page-settings-mqtt.js` Alpine store to enable eventual elimination of monolithic settings.html

**FOLLOW ESTABLISHED PATTERNS:**
- Use `/settings/mqtt.html` page structure (copy from device.html/wifi.html as templates)
- Create `page-settings-mqtt.js` Alpine store following existing pattern in `page-settings-device.js` and `page-settings-wifi.js`
- Reference existing MQTT functionality in `settings-old.html` for feature requirements
- Must use semantic CSS classes from shared.css - NO long Tailwind strings
- Update build configs in `esbuild.config.js` and `package.json` (follow device/wifi examples)

**CRITICAL**: All new pages must support light/dark mode and use established CSS architecture. Test thoroughly before proceeding.

- [ ] Create `/settings/mqtt.html` using data-driven config pattern (reference: `settings-old.html` MQTT section)
- [ ] Create `page-settings-mqtt.js` Alpine store (follow page pattern from Step 3.1)
- [ ] Implement MQTT connection testing with loading states and timeout handling
- [ ] Add server/port/credentials form with validation using existing API endpoints, secure credential handling
- [ ] Add enable/disable toggle with connection status feedback and error handling
- [ ] Update `esbuild.config.js` with mqtt build config (as we did for device page)
- [ ] Update `package.json` build scripts to include mqtt page bundles
- [ ] Test against existing MQTT functionality in settings-old.html for feature parity
- [ ] Run Testing Workflow

### Step 3.5: Unbidden Ink Settings Page
**Problem**: Unbidden Ink configuration exists in monolithic settings.html with mixed concerns - single file contains all settings sections and JavaScript
**Solution**: Extract existing Unbidden Ink interface (API token, scheduling, autoprompts) from settings-old.html into dedicated `/settings/unbidden.html` with separate `page-settings-unbidden.js` Alpine store to enable eventual elimination of monolithic settings.html
**Risks**: API token security, scheduling validation, autoprompt selection, existing functionality preservation
**Success Criteria**: API token validation works, scheduling configuration functional, autoprompt selection preserved, no regressions from settings-old.html

- [ ] Create `/settings/unbidden.html` using data-driven config pattern (reference: `settings-old.html` Unbidden Ink section)
- [ ] Create `page-settings-unbidden.js` Alpine store (follow page pattern from Step 3.1)
- [ ] Implement API token validation with secure handling and feedback
- [ ] Add scheduling configuration form with time validation using existing API endpoints
- [ ] Add autoprompt selection with preview functionality and error handling
- [ ] Update `esbuild.config.js` with unbidden build config (as we did for device page)
- [ ] Update `package.json` build scripts to include unbidden page bundles
- [ ] Test against existing Unbidden Ink functionality in settings-old.html for feature parity
- [ ] Run Testing Workflow

### Step 3.6: Button Settings Page
**Problem**: Button configuration exists in monolithic settings.html with mixed concerns - single file contains all settings sections and JavaScript
**Solution**: Extract existing button interface (action dropdowns, testing functionality) from settings-old.html into dedicated `/settings/buttons.html` with separate `page-settings-buttons.js` Alpine store to enable eventual elimination of monolithic settings.html
**Risks**: Button action validation, testing functionality, action dropdown population, existing functionality preservation
**Success Criteria**: Button action selection works, testing functionality preserved, action validation maintained, no regressions from settings-old.html

- [ ] Create `/settings/buttons.html` using data-driven config pattern (reference: `settings-old.html` Button section)
- [ ] Create `page-settings-buttons.js` Alpine store (follow page pattern from Step 3.1)
- [ ] Implement button action dropdowns with validation using existing API endpoints
- [ ] Add button testing functionality with feedback and error handling
- [ ] Add action validation with user-friendly messages
- [ ] Update `esbuild.config.js` with buttons build config (as we did for device page)
- [ ] Update `package.json` build scripts to include buttons page bundles
- [ ] Test against existing button functionality in settings-old.html for feature parity
- [ ] Run Testing Workflow

### Step 3.7: LED Settings Page
**Problem**: LED configuration exists in monolithic settings.html with mixed concerns - single file contains all settings sections and JavaScript
**Solution**: Extract existing LED interface (effects, colors, brightness, testing) from settings-old.html into dedicated `/settings/leds.html` with separate `page-settings-leds.js` Alpine store to enable eventual elimination of monolithic settings.html
**Risks**: LED effect testing, color picker functionality, brightness control, existing functionality preservation
**Success Criteria**: LED effect testing works, color/brightness controls functional, effect selection preserved, no regressions from settings-old.html

- [ ] Create `/settings/leds.html` using data-driven config pattern (reference: `settings-old.html` LED section)
- [ ] Create `page-settings-leds.js` Alpine store (follow page pattern from Step 3.1)
- [ ] Implement LED effect selection with preview and testing functionality
- [ ] Add color picker and brightness controls using existing API endpoints
- [ ] Add effect testing with feedback and error handling
- [ ] Update `esbuild.config.js` with leds build config (as we did for device page)
- [ ] Update `package.json` build scripts to include leds page bundles
- [ ] Test against existing LED functionality in settings-old.html for feature parity
- [ ] Run Testing Workflow

### Step 3.8: Memo Settings Page
**Problem**: Memo configuration exists in monolithic settings.html with mixed concerns - single file contains all settings sections and JavaScript
**Solution**: Extract existing memo interface (content editor, save/load, management) from settings-old.html into dedicated `/settings/memos.html` with separate `page-settings-memos.js` Alpine store to enable eventual elimination of monolithic settings.html
**Risks**: Content editor functionality, save/load operations, memo management, existing functionality preservation
**Success Criteria**: Memo editor works, save/load operations functional, content management preserved, no regressions from settings-old.html

- [ ] Create `/settings/memos.html` using data-driven config pattern (reference: `settings-old.html` Memo section)
- [ ] Create `page-settings-memos.js` Alpine store (follow page pattern from Step 3.1)
- [ ] Implement memo content editor with validation and character limits
- [ ] Add save/load functionality using existing API endpoints with error handling
- [ ] Add memo management (create/edit/delete) with user feedback
- [ ] Update `esbuild.config.js` with memos build config (as we did for device page)
- [ ] Update `package.json` build scripts to include memos page bundles
- [ ] Test against existing memo functionality in settings-old.html for feature parity
- [ ] Run Testing Workflow

### Step 3.9: Navigation & Cleanup
**Problem**: Settings pages exist in isolation without proper navigation, monolithic settings.html still present creating maintenance burden
**Solution**: Add navigation between settings pages and remove monolithic files once all individual pages are functional
**Risks**: Navigation breaking, links pointing to removed files, user confusion during transition, functionality gaps
**Success Criteria**: All settings pages accessible via navigation, monolithic files safely removed, no broken links, user experience maintained

- [x] Create overview page with section links (`/settings.html` with navigation grid)
- [ ] Add client-side navigation between pages with breadcrumbs and back buttons
- [ ] Verify all individual settings pages are fully functional and tested
- [ ] Remove monolithic settings.html and settings-old.html files
- [ ] Clean up unused partials and legacy code
- [ ] Update all internal links to point to new settings pages
- [ ] Test complete settings workflow end-to-end for regressions
- [ ] Run Testing Workflow

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

1. **Loading States**: Wait till page ready, fade in with opacity transition. No "Loading" text or spinners.
2. **Visual Hierarchy**: Use accent colors for section headers, not background boxes.
3. **Save Button**: Only enabled when changes detected. Use Alpine reactive `canSave` getter with `hasChanges()` logic.
4. **Tab Interfaces**: Proper tab styling with visual connection to content panel (border, background).

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

### Success Criteria
- All settings functionality identical to current (settings-old.html)
- Code more maintainable and modular
- No regressions in UX, only improvements.
- ESP32 memory usage unchanged

## Lessons Learned 📝
```
**Key Learning:** Always use Alpine's built-in reactivity (`x-effect`, `$nextTick`) instead of custom solutions

**Key Learning:** Maintain parallel versions during major architectural changes
```

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