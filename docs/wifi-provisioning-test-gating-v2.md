# WiFi Provisioning Test Gating Plan

Status: Draft (Updated Plan)  
Scope: AP provisioning flow (`setup.html`) — NOT applied to STA maintenance page (`wifi.html`) in initial implementation.

## Quick Task Plan (Categorized)

Execution checklist grouped by system:

ESP32 Live Backend (Firmware + Web Server)

- [ ] AP fallback mode: change `startFallbackAP()` in `src/core/network.cpp` to `WiFi.mode(WIFI_AP_STA)`; call `WiFi.softAP(...)` immediately after.
- [ ] Routing allowlist: in `src/web/web_server.cpp` add `/api/test-wifi` to `handleCaptivePortal` allowlist (AP mode).
- [ ] Route registration (AP mode): in `src/web/web_server.cpp` register `server.on("/api/test-wifi", HTTP_POST, handleTestWiFi)` inside the AP setup block.
- [ ] Endpoint declaration: add `void handleTestWiFi(AsyncWebServerRequest *request);` to `src/web/api_system_handlers.h`.
- [ ] Endpoint implementation: in `src/web/api_system_handlers.cpp` implement blocking test with mutex, 6.5s poll loop (`delay(75)` + `yield()` + `esp_task_wdt_reset()`), success RSSI, `WiFi.disconnect()`, and structured responses (200/400/408/409/422) with temporary WiFi event handlers for classification.

- Front-End (HTML/CSS/JS)

- [ ] API: add `testWiFiConnection(ssid, password)` (POST `/api/test-wifi`) to `src/data/js/api/setup.js`.
- [ ] Store: in `src/data/js/stores/setup.js` add state (`wifiTesting`, `wifiTestResult`, `wifiTestPassed`, `dirtySinceLastTest`), methods (`resetWifiTestState`, `markDirtyOnCredentialChange`, `testWifiConnection`), add Alpine `$watch` on SSID/password/mode changes to call `markDirtyOnCredentialChange`, and update `canSave` to require `wifiTestPassed && !dirtySinceLastTest`.
- [ ] UI: in `src/data/setup.html` add a “Test WiFi” button with spinner, a status line, a gating banner, and disable scan while testing. Gate UI with `window.FEATURE_WIFI_TEST` (default true).
- [ ] Docs: add a brief reference entry in `docs/frontend-patterns.md` describing the gating pattern and linking to this spec.
- [ ] Shared utilities: do NOT split `src/data/js/device-config-utils/wifi-utils.js`. Keep it as the shared scan/state/validation layer used by both setup.html and wifi.html. Provisioning-specific logic lives in the setup store only.

Mock Server (Node.js)

- [ ] Implement `/api/test-wifi` in `mock-server/mock-api.js` under the `/api/*` block (before any captive-portal/static routing). Accept POST `{ ssid, password }`, simulate success/failure and 409 Busy with realistic delays, and return payloads identical to the firmware.
- [ ] Preserve route precedence: ensure `/api/*` routes are processed before AP captive-portal redirects so `/api/test-wifi` works when running with `--ap-mode`.

Verification

- [ ] Valid creds → Test → Success → Save enabled; mutate password → Save disabled until retest.
- [ ] Invalid creds → Test → Fail → Save disabled.
- [ ] Open network: blank password path works.
- [ ] AP remains reachable during/after tests (disconnect does not drop AP).

Rollback

- [ ] Toggle off UI: set `window.FEATURE_WIFI_TEST = false` to remove gating quickly.
- [ ] Revert AP fallback change: switch `WIFI_AP_STA` back to `WIFI_AP` in `startFallbackAP()` if AP stability issues occur.

## 1. Objective

Ensure first-time WiFi credentials are **validated (association + auth)** before they're persisted and the device restarts. Prevents user lock-out from mistyped SSID/password during initial provisioning.

## 1.1 Current vs Changes

- Current: On boot with valid credentials, device connects as `WIFI_STA`; on failure or no credentials, enters AP fallback as `WIFI_AP` (AP-only) with captive portal and a minimal route set. `/api/wifi-scan` exists; the setup UI has no WiFi test gating. Saving in AP mode via `/api/setup` switches the device to `WIFI_STA` (AP stops).
- Change (DO THIS): In AP fallback, use `WIFI_AP_STA` instead of `WIFI_AP`. Add a blocking `/api/test-wifi` endpoint. Allowlist (mock and live) and register `/api/test-wifi` in AP setup mode. Add FE test state with a dirty-flag gating on `setup.html`; disable scanning while testing. Protect the endpoint with a mutex. Use `WiFi.disconnect()` (not `true`). Add temporary WiFi event handlers for error classification. Add `window.FEATURE_WIFI_TEST` (default true). Update frontend patterns doc and the mock server with `/api/test-wifi`.

## 2. Design Principles

- **Boot behavior**: On boot, the device attempts normal STA connection if WiFi is configured. If STA connects, the device stays in `WIFI_STA` (no AP). If STA fails to connect (or credentials are missing), it enters AP fallback using `WIFI_AP_STA`.
- **No duplication**: Reuse existing shared scan + state logic in `wifi-utils.js` (centralizes scanning/network processing). Do not split this module.
- **Provisioning-only gating**: Gate saving on `setup.html` only. Do not gate the normal WiFi settings page (STA mode) to keep it fast and responsive.
- **Non-persistent test**: Test does _not_ write to NVS; final save still goes through existing `/api/setup` POST.
- **Minimal firmware footprint**: Start with a blocking endpoint. No async + polling.
- **Deterministic invalidation**: ANY credential mutation after a successful test invalidates the pass state.
- **Stable dual-mode (AP_STA) during provisioning**: In AP fallback mode, the device runs `WIFI_AP_STA` for the entire setup session (AP provides captive portal; STA used for scan + test). After successful credential save and verified STA connection, drop AP (switch to `WIFI_STA`). This eliminates mid-process mode churn (was AP → AP_STA → AP) and yields faster, more reliable tests.
  - Keep normal boot as `WIFI_STA` when credentials are present; switch to AP fallback as `WIFI_AP_STA` only when STA fails or credentials are missing.

### 2.1 Backend Prerequisite (AP Mode Initialization)

Current state: On failure to connect (or no credentials), AP fallback initializes WiFi with `WIFI_AP`. Update the AP fallback path to use dual mode `WIFI_AP_STA`.

Required adjustments (DO THIS):

1. In `startFallbackAP()`, replace `WiFi.mode(WIFI_AP)` with `WiFi.mode(WIFI_AP_STA)`.
2. Call `WiFi.softAP(...)` immediately after setting the mode.
3. Keep `/api/wifi-scan` unchanged; scanning works in AP_STA. Keep the captive portal and DNS responder unchanged.

Acceptance checks for this prerequisite:

- When STA connects at boot: device stays in `WIFI_STA` (no AP visible).
- When STA fails or credentials are missing: AP SSID is visible and the device runs in `WIFI_AP_STA`.
- Scan endpoint returns results normally.
- Memory / heap deltas acceptable (STA radio context adds a small constant footprint).
- No regressions in button / other peripherals initialization timing.

Rollback: revert the single line back to `WIFI_AP` if any side-effect is observed (none anticipated).

## 3. Front-End Additions (setup store)

New transient fields:

```
wifiTesting: boolean
wifiTestResult: { success: boolean, message: string } | null
wifiTestPassed: boolean
dirtySinceLastTest: boolean
```

Invalidation triggers (set `dirtySinceLastTest = true` and `wifiTestPassed = false`):

- SSID radio change (scan mode)
- Switching to manual mode / editing manual SSID
- Password input change
- Network rescan followed by new selection
- Mode toggle scan ↔ manual

`canSave` logic (setup only):

```
canSave = baseFormValid && wifiTestPassed && !dirtySinceLastTest
```

Base form validity = owner + timezone + effectiveSSID + password all non-empty (existing logic retained).

### Methods to Add

```
resetWifiTestState()
markDirtyOnCredentialChange()
async testWifiConnection()  // calls POST /api/test-wifi
```

### UI (setup.html)

Components to insert below password field:

- Primary button: "Test WiFi"
  - States: Idle | Testing (spinner) | WiFi connected (green check) | Connection failed (red text persists until change)
- Status line: displays last `wifiTestResult.message` or guidance
- Banner (small, subtle) when form valid but not yet tested: “Please test WiFi before saving.”
- Save button tooltip reasons: missing fields vs. "Test WiFi first".

No other structural changes; scanning & password UI untouched.

## 4. Firmware Endpoint (blocking)

`POST /api/test-wifi`
Body:

```
{ "ssid": "...", "password": "..." }
```

Process (with AP_STA already active):

1. Confirm we are in `WIFI_AP_STA` (expected while in AP fallback after Section 2.1).
2. `WiFi.begin(ssid, password)`.
3. Poll `WiFi.status()` for up to 6.5s, sleeping 75ms per iteration (max 87 iterations). **Watchdog Safety:**

- Call `delay(75)` and `yield()` each loop to service background tasks.
- Keep total blocking window < 8s (Arduino core WDT margin). Abort early if `WL_NO_SSID_AVAIL` or `WL_CONNECT_FAILED` occurs.

4. Success if `WL_CONNECTED` reached; capture RSSI, then immediately call `WiFi.disconnect()` (without `true`) to disconnect STA only. This preserves AP service continuity and ensures credentials aren't considered “active” yet. Avoid `WiFi.disconnect(true)` which powers off WiFi on ESP32 and would kill the AP.
5. Return JSON (status mapping):

- 200 `{ success: true, rssi: -52 }` (connected within timeout)
- 408 `{ success: false, message: "Association timeout" }` (no connection within 6.5s)
- 400 `{ success: false, message: "Authentication failed" | "No AP found" | "Network error" }` (classified via events)
- 409 `{ success: false, message: "Test already running" }` (concurrency guard)
- 422 `{ success: false, message: "Invalid payload" }` (missing/invalid ssid/password)

6. Zero out password buffer.

### Concurrency Guard (Required)

Enforce a single in‑flight test using a FreeRTOS mutex (or critical section). Do not rely on a `volatile` flag.

Guard behavior:

1. On entry: if locked → respond `409` `{ success:false, message:"Test already running" }`.
2. Acquire the mutex only after payload validation.
3. Release the mutex in all exit paths (success, timeout, early error) using RAII or a `finally` pattern.
4. If a test exceeds 10s, forcibly release and return a timeout.

Final provisioning (outside this test): after user presses Save and credentials are persisted + a full real connect succeeds, switch to `WIFI_STA` to drop AP. This transition happens once, not during tests.

### Watchdog / Crash Avoidance Notes

- Do **not** spin with a tight busy loop; always use `delay()` (or at least `yield()`) so background tasks run.
- Avoid calling fresh WiFi scans inside the connect poll loop; reuse the prior scan results.
- Guard against NULL / empty SSID or excessively long password before `WiFi.begin`.
- Channel drift issues are rarer when staying continuously in `WIFI_AP_STA`; if observed after a partial connect attempt, re-init SoftAP (defensive fallback).
- Avoid `WiFi.disconnect(true)` in tests; it turns off WiFi and will drop the AP.

### Failure Messages (Use This Set)

- `Association timeout` (never reached connected state)
- `Authentication failed` (auth error event if available)
- `No AP found` (ESP32 scan mismatch / event)
- `Network error` (generic fallback)

Implementation (DO THIS): Install temporary WiFi event handlers to classify failures (auth vs. no AP). Also implement a conservative timeout with a generic fallback message.

## 5. Security & Safety

- Never persist credentials during test.
- Do not echo password back.
- Wipe password staging buffer after attempt.
- Maintain AP so user doesn’t lose access mid-provisioning.

## 6. Edge Cases

| Case                             | Handling                                                                                     |
| -------------------------------- | -------------------------------------------------------------------------------------------- |
| Open network (no password)       | Allow; empty password handled normally; gating unaffected                                    |
| Hidden SSID manual entry         | Treat like manual path; test as normal                                                       |
| Rapid repeated clicks            | Disable button while `wifiTesting`                                                           |
| Scan during test                 | Disable scan button while `wifiTesting`                                                      |
| Very weak RSSI                   | Still allow; success message may include RSSI                                                |
| User changes owner/timezone only | Does NOT invalidate WiFi test                                                                |
| Near watchdog threshold          | Abort test at ~6.5s, return timeout message (preserves WDT margin)                           |
| Intermittent connect-then-drop   | Treat as fail unless stable `WL_CONNECTED` for >1 poll cycle                                 |
| Captive portal interaction       | Ensure `/api/test-wifi` is allowlisted in AP captive portal and registered in AP route block |

## 7. Testing Strategy

### Front-End (Mock)

- Mock 200 success → success state; then mutate password → invalidates.
- Mock 400 fail → error message + gating persists.
- Simulate network error (reject fetch) → generic failure.

### Device Integration

1. Enter AP setup → fill credentials (valid) → Test → Success → Save enabled.
2. Change password → Save disabled until retest.
3. Enter invalid password → Test → Fail → Save disabled.
4. Open network: blank password path.
5. Confirm AP remains reachable during/after test (disconnect does not drop AP).

## 8. Implementation Sequence (Recommended)

0. Firmware (DO THIS): update AP fallback to `WIFI_AP_STA` (see Section 2.1). Verify STA boot path unchanged; verify AP + scan + portal stability when STA fails.
1. Firmware (DO THIS): add blocking `/api/test-wifi` + structured logs (tag: WIFI_TEST).
   1a. Firmware (DO THIS): allowlist `/api/test-wifi` in captive portal and register its route in the AP setup block.
2. Front-end (DO THIS): add store state + methods; gate UI with `window.FEATURE_WIFI_TEST` (default true) to permit instant rollback if needed.
3. Front-end (DO THIS): inject UI components in `src/data/setup.html` (button, spinner, status line, banner).
4. Device test (DO THIS): manual smoke test including retest after intentional credential change.
5. Docs (DO THIS): update `docs/frontend-patterns.md` with a reference entry.
6. Keep settings page lean: do not add a test button to `wifi.html` (non-gated). Revisit only with a strong UX case.

## 9. Success Criteria

- User cannot submit provisioning save with untested or altered WiFi credentials.
- Successful test never soft-bricks access (AP stays reachable).
- Retest requirement is always enforced on any SSID/password mutation.
- No measurable regression in initial setup load time.

## 10. Minimal Code Touch Points Summary

| Area                | File(s)                              | Change Type                                                              |
| ------------------- | ------------------------------------ | ------------------------------------------------------------------------ |
| Store               | `src/data/js/stores/setup.js`        | Add transient WiFi test state + methods + canSave augmentation           |
| API (FE)            | `src/data/js/api/setup.js`           | Add `testWiFiConnection()` that POSTs `/api/test-wifi`                   |
| UI                  | `src/data/setup.html`                | Insert test button, result text, banner (no structural rewrites)         |
| Firmware (endpoint) | `src/web/api_system_handlers.h/.cpp` | Add `/api/test-wifi` handler (blocking)                                  |
| Firmware (AP mode)  | `src/core/network.cpp`               | Change AP fallback to `WiFi.mode(WIFI_AP_STA)`                           |
| Firmware (routing)  | `src/web/web_server.cpp`             | Allowlist `/api/test-wifi` in captive portal; register route in AP block |
| Mock                | `mock-server/mock-api.js`            | Add stub for `/api/test-wifi` to support FE development                  |
| Docs                | `docs/frontend-patterns.md`          | Add reference section now                                                |

## 11. UX Rollback Strategy

If the blocking test proves unreliable:

- Hide button & gating behind `window.FEATURE_WIFI_TEST` flag; default false to revert instantly.
- Endpoint remains inert when unused; no data model changes to revert.
- If AP stability issues are observed, revert AP fallback from `WIFI_AP_STA` back to `WIFI_AP` (single-line change) and disable test feature.

## 12. Mock Server Parity Notes

- The mock server must mirror live routing behavior: all `/api/*` routes are handled before any AP-mode captive portal redirects. Keep this ordering intact so `/api/test-wifi` works in AP mode without extra allowlisting.
- Return payloads that match firmware shapes exactly:
  - 200 `{ success: true, rssi: -52 }`
  - 400/408 `{ success: false, message: "Association timeout" | "Authentication failed" | "No AP found" | "Network error" }`
  - 409 `{ success: false, message: "Test already running" }`
  - 422 `{ success: false, message: "Invalid payload" }`
- Use timeouts that reflect the device: keep under 2s for dev ergonomics and provide a toggle to simulate a ~6.5s timeout path when testing long‑running behavior.

## 13. Front-End Module Boundaries (Shared vs Provisioning-only)

- Keep `src/data/js/device-config-utils/wifi-utils.js` as the shared layer for:
  - `formatSignalStrength`, `deduplicateNetworks`, `sortNetworks`, `processNetworks`
  - `createWiFiState` (reactive scan/manual state), `performWiFiScan`
  - `getEffectiveSSID`, `validateWiFiConfig`
- Do NOT add provisioning state or API calls to `wifi-utils.js`.
- Implement provisioning-only logic in `src/data/js/stores/setup.js`:
  - Gating state (`wifiTesting`, `wifiTestResult`, `wifiTestPassed`, `dirtySinceLastTest`)
  - Methods (`markDirtyOnCredentialChange`, `testWifiConnection`)
  - `canSave` gating requiring a passed test and `!dirtySinceLastTest`
- Settings page (`src/data/js/stores/settings-wifi.js`) remains focused on password masking, timeout validation, and save logic; no gating added.
