# App Refactor Plan

Status date: 2025-09-03

Goal: Modernize and modularize the web + firmware-facing assets without regressions while keeping the ESP32 footprint lean.

## 0. Snapshot Summary

| Phase | Area | Status | Notes |
|-------|------|--------|-------|
| 1 | Build System | ✅ Complete | esbuild, env targets, mock server |
| 2 | Internal Organization | ✅ Complete | Separated concerns & utilities scaffold |
| 3 | Settings Refactor | ✅ Complete | All pages modular & tested live |
| 4 | Frontend Optimization | ✅ Complete | CSS unification, gzip, modules |
| 5 | Local Fonts / FOUC Removal | ✅ Complete | Outfit locally hosted |
| 6 | Remaining Pages | ⏳ Partial | Setup WiFi UX polish outstanding |
| 7 | Docs & Test Harness | 🚧 In Progress | Consolidate patterns + add missing tests |
| 8 | Firmware/Web Integration | 📝 Planned | Static asset serving + cache headers |
| 9 | Quality & Observability | 📝 Planned | Metrics, logging surfacing, perf budgets |
| 10 | Release & Maintenance | 📝 Planned | Versioning, artifact pipeline |

Focused Next Action: Finish Phase 6.2 (Setup page UX alignment) then lock patterns in Phase 7 documentation before adding new feature scope.

---

## Phase 1: Build System Modernization (Complete)
Delivered: esbuild integration (≈3× faster), dev/prod naming, mock server workflow.

## Phase 2: Internal Organization (Complete)
Delivered: Separated partials, reorganized API helpers, prepared utility structure.

## Phase 3: Settings Refactor (Complete)
Highlights:
* Color‑coded themed pages
* Partial (section‑scoped) config updates
* Consistent Alpine Simple Loading Flag Pattern
* Verified on mock + live ESP32

Pages Delivered: device, wifi, mqtt, memos, buttons, leds, unbidden ink, overview. Legacy monolith removed; link integrity verified.

## Phase 4: Optimization (Complete)

### 4.1 & 4.2 Large File + Purge Audit
timezones.json & riddles.ndjson retained (essential). Tailwind purge paths corrected and validated.

### 4.3 CSS Architecture
Problem: ~300KB duplicate base across 5 files.
Resolution: Single app.css (minified 125KB → 17KB gz). ~77% payload reduction.

### 4.4 GZIP Pipeline
Pre-compress build step, smart skip <1KB, mock server gzip headers, ready for ESP32 static serving.

### 4.5 ES6 Module System
All 13 legacy bundles migrated to ES6 sources → IIFE output (no runtime deviation). Improved IDE ergonomics & tree‑shaking headroom.

## Phase 5: Local Font Implementation (Complete)
Outfit hosted locally, FOUC eliminated via guarded templates + transitions. Formatting automation in place.

## Phase 6: Remaining Pages (Complete)
Delivered:
* 404 page aligned & converted
* Diagnostics split (overview + subpages)
* Setup (AP mode) updated with canonical guards & parity
* Navigation / UX consistency across non-settings pages

Exit Criteria Met: Safe guards, UX parity, no layout flashes, AP vs STA distinctions retained.

Post-Phase 6 Immediate Focus (before Phase 8 work):
1. CLAUDE doc modularization
2. WiFi settings connection test flow
3. NVS persistence audit & remediation

## Phase 7: Documentation & Test Harness (In Progress)
Objectives:
* Canonicalize patterns (loading, error handling, config mutation, modular store layout)
* Add/update docs: configuration-system.md (partial updates), logging-system.md (diagnostics routing), testing.md (mock + live flow)
* Ensure PlatformIO tests cover: config validation edge cases, memo handler boundaries, time utils DST edge, NVS persistence, web validation
* Add lightweight front-end integration smoke test (mock server) script (Node) for regression gate

Open Items:
[] Add pattern summary section to docs/testing.md linking to this plan
[] Script: node scripts/smoke_frontend.js (fetch critical endpoints + assert 200 + gzip header)
[] Expand test_config_validation.cpp for malformed partial payloads
[] Add README snippet for asset compression rationale

## Phase 8: Release & Maintenance (Planned)
* Formal semantic version stamp (web + firmware alignment)
* Automated build artifact bundle: firmware.bin + data/ (gz assets) + CHANGES summary
* Checklist: build → unit tests → smoke_frontend → size audit → tag → release
* Potential GitHub Action (future) if remote CI introduced

---

## Front-End Pattern Reference

Detailed Alpine.js patterns (error card, x-if + x-show rationale, empty object approach, single init guard, audit checklist) live in: `docs/frontend-patterns.md`.

This plan intentionally keeps only phase tracking; do not reintroduce verbose examples here.

---

## Open Action Checklist (Living)
[] Split `CLAUDE.md` into nested versions (overview, hooks, automation, patterns)
[] WiFi settings: add connection test button + save gating (mirrors MQTT pattern)
[] NVS persistence: inventory runtime vs stored; persist missing critical keys (e.g. printer TX pin) + test
[] Add smoke_frontend test harness script
[] Extend config validation unit tests (malformed partial payloads)
[] Asset serving: implement gzip + cache headers on ESP32
[] Diagnostics: add heap + build info surfacing
[] Document release checklist in README or new RELEASE.md

Completion Definition (Refactor Scope): All phases ≤7 DONE, 8–10 minimally bootstrapped with first pass implementations, and repeatable release procedure documented.

---

## Decision Log (Recent)
* Retained large JSON + NDJSON assets: value > optimization risk
* Chose single app.css over per-page layering to shrink cache footprint
* Adopted direct assignment pattern (config subsections) for clarity & memory
* Declined dynamic on-device compression (pre-build gzip cheaper + simpler)

---

This document should stay tactical: update statuses and the Open Action Checklist as tasks land; avoid reintroducing verbose examples already documented elsewhere.
