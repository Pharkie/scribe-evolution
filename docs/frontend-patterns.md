# Frontend Patterns Guide

Status date: 2025-09-03

Purpose: Canonicalize UI architecture patterns (Alpine.js + ESP32 constraints) so new pages/features remain consistent, resilient, and lightweight.

## 1. Core Principles
- Single responsibility per page store
- Lazy population: start with empty objects, not deep null scaffolds
- Deterministic state machine: (idle → loading → loaded|error)
- Template safety via `x-if` outer gate + inner animated wrapper
- Idempotent initialization guarded by `initialized` boolean
- Direct assignment of only the configuration branches the page actually renders

## 2. Canonical Store Skeleton
```js
function createStore() {
  return {
    loaded: false,        // true when a result (success or error) is ready
    error: null,          // string | null
    initialized: false,   // prevents double fetch
    config: {},           // lazily populated
    async loadConfiguration() {
      if (this.initialized) return;     // idempotent
      this.initialized = true;
      this.loaded = false;
      this.error = null;
      try {
        const r = await fetch('/api/config');
        if (!r.ok) throw new Error(`HTTP ${r.status}`);
        const data = await r.json();
        // Assign ONLY what this page needs:
        this.config.mqtt = {
          enabled: !!data.mqtt?.enabled,
          server: data.mqtt?.server || '',
          port: data.mqtt?.port ?? 1883,
          username: data.mqtt?.username || '',
          password: data.mqtt?.password || ''
        };
        this.loaded = true;
      } catch (e) {
        this.error = e.message;
        this.loaded = true; // Important: allow error template to appear
      }
    }
  };
}
```

## 3. Template Guard Pattern
```html
<body x-data="$store.mqtt" x-init="loadConfiguration()">
  <!-- Success Content -->
  <template x-if="loaded && !error">
    <div
      x-data="{show:false}"
      x-init="$nextTick(()=>show=true)"
      x-show="show"
      x-transition.opacity.duration.250ms
    >
      <!-- SAFE references to config.* go here -->
    </div>
  </template>

  <!-- Error Card -->
  <template x-if="loaded && error">
    <div
      role="alert"
      class="rounded-2xl border border-red-300 dark:border-red-800 bg-red-50 dark:bg-red-900/25 p-5 flex flex-col gap-3 text-sm"
    >
      <div class="flex items-center gap-2 font-semibold text-red-800 dark:text-red-200">
        <svg class="w-5 h-5" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
          <path d="M12 9v4m0 4h.01" />
          <circle cx="12" cy="12" r="9" />
        </svg>
        <span>Error Loading Settings</span>
      </div>
      <p class="text-red-700 dark:text-red-300" x-text="error"></p>
      <div class="flex gap-2">
        <button
          type="button"
          class="px-3 py-1.5 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-xs font-medium"
          @click="initialized=false; loadConfiguration()"
        >Retry</button>
      </div>
    </div>
  </template>
</body>
```

## 4. x-if + x-show: Why Both?
| Concern | x-if | x-show | Combined Benefit |
|---------|------|--------|------------------|
| Avoid evaluating undefined properties | ✅ | ❌ | Prevent crashes before data arrives |
| Animation friendliness | Manual re-insert | ✅ | Smooth fade while staying safe |
| Memory churn | Removes subtree | Keeps subtree | Safety outside + animation inside |
| Initial paint FOUC avoidance | ✅ (no DOM) | ❌ (DOM hidden) | Outer gate hides, inner animates |

Pattern layering:
```html
<template x-if="loaded && !error">
  <div x-data="{show:false}" x-init="$nextTick(()=>show=true)" x-show="show" x-transition.opacity>
    <!-- content -->
  </div>
</template>
```

## 5. Empty Object vs Large Null Graph
Anti‑pattern:
```js
config: {
  device: { owner: null, timezone: null },
  mqtt: { enabled: null, server: null, port: null, username: null, password: null },
  buttons: { b1: { gpio: null, short: null, long: null } }
}
```
Problems:
1. Drift: backend changes silently orphan fields.
2. Memory: wasted baseline allocations.
3. Brittleness: templates assume presence prematurely.
4. Complexity: harder partial update payload construction.

Preferred:
```js
config: {};
// Later in loadConfiguration
this.config.mqtt = { enabled: !!d.mqtt?.enabled, server: d.mqtt?.server || '' };
```
Benefits: minimal footprint, explicit ownership, resilient to schema evolution.

Rule: Only assign branches the current page renders.

## 6. Single Initialization Guard
Rules:
1. Exactly one public method: `loadConfiguration()`.
2. `initialized` boolean prevents double fetch.
3. Retry resets `initialized=false` first.
4. `loaded` flips true in success AND error paths.
5. Avoid alias methods (`init`, `start`) to reduce drift.

Retry pattern (button):
```html
@click="initialized=false; loadConfiguration()"
```

Common mistakes:
- Calling `loadConfiguration()` in multiple Alpine lifecycle hooks.
- Not marking `loaded=true` on error → perpetual spinner.
- Replacing `this.config = {}` after binding established → breaks reactivity references (prefer mutating subsections).
- Adding deep unused branches “just in case.”

## 7. Error Card Guidelines
Checklist:
- `role="alert"` present
- Distinctive border + background (light + dark modes)
- Specific actionable message (avoid generic "Error")
- Retry resets `initialized`
- No console.log side effects needed for normal failures

Optional enhancements:
- Add timestamp of failure
- Add last successful load time outside card

## 8. Quick Audit Checklist (Per Page)
[] Uses outer x-if for success and error templates
[] Inner animated container uses x-show + transition
[] `loaded` true on both success and error paths
[] No pre-built null graphs
[] Single initializer guarded by `initialized`
[] Retry resets `initialized`
[] No duplicate fetch triggers
[] Only assigns config subsections actually rendered

## 9. Performance & Footprint Notes
- Each additional always-present DOM node costs memory; keep outer template lean.
- Avoid large inline JSON dumps; fetch lazily if secondary panels required.
- Pre-gz pipeline means minimizing uncompressed size still matters (affects flash usage).

## 10. Example Minimal Page Diff (Before → After)
Before (problematic):
```js
config: { mqtt: { enabled: null, server: null } },
async init() { await this.fetchConfig(); },
async fetchConfig() { /* no error differentiation */ }
```
After (canonical):
```js
config: {},
async loadConfiguration() { /* guarded, sets loaded & error reliably */ }
```
Template change:
```html
<!-- Replace x-show wrapper with x-if + inner animated div -->
```

## 11. Pitfall Matrix
| Pitfall | Symptom | Fix |
|---------|---------|-----|
| Missing x-if | Alpine crashes on first paint | Wrap in `<template x-if="loaded && !error">` |
| No loaded flag on error | Infinite spinner | Set loaded=true in catch block |
| Reassign entire config | Lost bindings | Mutate branches: `this.config.device = {...}` |
| Multi init methods | Double network calls | Keep only `loadConfiguration` |
| Deep null graph | Verbose & fragile templates | Start empty, assign on success |

## 12. Copy/Paste Snippets
Store stub:
```js
export function createXStore() { /* clone canonical skeleton */ }
```
Error card retry line:
```html
@click="initialized=false; loadConfiguration()"
```
Animated safe wrapper:
```html
<template x-if="loaded && !error">
  <div x-data="{show:false}" x-init="$nextTick(()=>show=true)" x-show="show" x-transition.opacity>
    <!-- content -->
  </div>
</template>
```

---
Update this file when patterns evolve. Keep REFACTOR_PLAN.md concise and link here for the detailed rationale.
