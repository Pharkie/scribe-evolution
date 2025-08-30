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

### Next Steps: Remaining Settings Pages

**3.5 Unbidden Ink** - Extract AI content configuration (API token, scheduling, autoprompts)
**3.6 Button Settings** - Extract button action configuration with testing 
**3.7 LED Settings** - Extract LED effect configuration with preview
**3.8 Memo Settings** - Extract memo content editor
**3.9 Navigation** - Add page navigation and remove monolithic files

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

**4.0 REST API Refactor** - Convert endpoints to proper HTTP methods (GET/PATCH/PUT/POST)
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

## Phase 5: Future Planning 📋

- [ ] Document working patterns from settings refactor
- [ ] Create new refactor plan for index, diagnostics, 404 pages
- [ ] Define templates for page structure and API patterns

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

**ESP32 Constraints:**
- file.readString() fails on large files - use chunked reading for >8KB
- Work within buffer constraints - separate endpoints better than large JSON
- Don't overcomplicate native ESP32 functionality (WiFi scanning) - it worked before
- Single WiFi radio means scanning affects connection - let ESP32 handle natively
- MQTT TLS: Racing connections cause socket conflicts - use connection guard and recreate WiFiClientSecure

**Configuration Management:**
- Centralized config loading prevents inconsistencies - all settings use RuntimeConfig
- Maintain parallel versions during major architectural changes

---

## Rejected Ideas 🚫

**Partial Config GET Endpoints** - Creating `/api/config/device`, `/api/config/wifi` etc. for reading config sections. Over-engineering: full config JSON isn't large enough to warrant splitting, adds maintenance burden and memory overhead on ESP32. (Note: partial writes via POST `/api/config` already work fine.)

## Live ESP32 Testing Status 🔧

**✅ COMPLETED - Step 3.1: Device Settings**
**✅ COMPLETED - Step 3.3: WiFi Settings** 
**✅ COMPLETED - Step 3.4: MQTT Settings**
**✅ COMPLETED - Step 3.4.5: Timezone Picker**

All completed settings pages have been verified on live ESP32 hardware.

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