# WiFi Provisioning Test Gating Plan

Status: Draft (Updated Plan)  
Scope: AP provisioning flow (`setup.html`) — NOT applied to STA maintenance page (`wifi.html`) in initial implementation.

## 1. Objective
Ensure first-time WiFi credentials are **validated (association + auth)** before they're persisted and the device restarts. Prevents user lock-out from mistyped SSID/password during initial provisioning.

## 2. Design Principles
- **No duplication**: Reuse existing shared scan + state logic in `wifi-utils.js` (already centralizes scanning/network processing).
- **Provisioning only gating**: Mandatory test in AP setup flow; optional (or omitted) in normal WiFi settings (STA mode) to keep that page fast and flexible.
- **Non-persistent test**: Test does *not* write to NVS; final save still goes through existing `/api/setup` POST.
- **Minimal firmware footprint**: Start with a blocking endpoint. No async + polling.
- **Deterministic invalidation**: ANY credential mutation after a successful test invalidates the pass state.
- **Stable dual-mode (AP_STA) during provisioning**: In AP fallback mode, the device runs `WIFI_AP_STA` for the entire setup session (AP provides captive portal; STA used for scan + test). Only after successful credential save and verified STA connection do we optionally drop AP (switch to `WIFI_STA`). This eliminates mid-process mode churn (was AP → AP_STA → AP) and yields faster, more reliable tests.
  - NOTE: We do NOT need to set `WIFI_AP_STA` at boot. Instead, update the AP fallback path to use `WIFI_AP_STA` (minimal change, avoids altering normal STA boot).

### 2.1 Backend Prerequisite (AP Mode Initialization)
Current state: AP fallback initializes WiFi with `WIFI_AP`. Update only the AP fallback path to use dual mode.

Required adjustments:
1. In AP fallback (e.g., `startFallbackAP()`), replace `WiFi.mode(WIFI_AP)` with `WiFi.mode(WIFI_AP_STA)`.
2. Ensure SoftAP configuration (SSID, password, channel, hidden flag) still follows after mode set.
3. Verify existing scan logic still works (it will; scans in AP_STA are standard) and captive portal / DNS responder unaffected.

Acceptance checks for this prerequisite:
- AP SSID remains visible when entering AP fallback.
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
lastTestFingerprint: string | null
```

Fingerprint strategy (fast + side-effect free):
```
fingerprint = `${effectiveSSID}::${password.length}:${simpleXorHash(password)}`
```
If current fingerprint != lastTestFingerprint ⇒ test invalid (must retest).

Invalidation triggers:
- SSID radio change (scan mode)
- Switching to manual mode / editing manual SSID
- Password input change
- Network rescan followed by new selection
- Mode toggle scan ↔ manual

`canSave` logic (setup only):
```
canSave = baseFormValid && wifiTestPassed && !fingerprintChanged
```
Base form validity = owner + timezone + effectiveSSID + password all non-empty (existing logic retained).

### Methods to Add
```
computeFingerprint() -> string
resetWifiTestState()
invalidateWifiTestIfNeeded()
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
3. Poll `WiFi.status()` up to ~6–8s (e.g. 80 × 75ms with `delay(75)`). **Watchdog Safety:**
  - Each `delay(75)` yields; optional extra `yield();` inside loop is fine but not required.
  - Keep total blocking window < 8s (typical Arduino core WDT margin); abort early if `WL_NO_SSID_AVAIL` or `WL_CONNECT_FAILED` surfaced.
  - Hard cap: break at 6.5s and mark timeout rather than stretching to full 8s to retain margin.
4. Success if `WL_CONNECTED` reached; capture RSSI, then immediately call `WiFi.disconnect()` (without `true`) to disconnect STA only. This preserves AP service continuity and ensures credentials aren't considered “active” yet. Avoid `WiFi.disconnect(true)` which powers off WiFi on ESP32 and would kill the AP.
5. Return JSON:
  - 200 `{ success: true, rssi: -52 }`
  - 400 `{ success: false, message: "Association timeout" }`
6. Zero out password buffer.

### Concurrency Guard (Required)
Single in-flight test enforced by a guard:
- Preferred: a FreeRTOS mutex or critical section to ensure mutual exclusion under AsyncWebServer callbacks.
- Minimal fallback: a module‑scope `volatile bool wifiTestBusy` with strict set/clear discipline.

Guard behavior:
1. On entry: if busy → respond `409` `{ success:false, message:"Test already running" }`.
2. Set busy only after payload validation.
3. Clear in all exit paths (success, timeout, early error) using RAII helper or `finally` pattern.
4. (Optional) If busy stuck > 10s (stale), allow next call to override.

Final provisioning (outside this test): after user presses Save and credentials are persisted + a full real connect succeeds, optionally switch to `WIFI_STA` to drop AP if product policy prefers (or keep AP_STA if ongoing portal features are desired). This transition happens once, not during tests.

### Watchdog / Crash Avoidance Notes
- Do **not** spin with a tight busy loop; always use `delay()` (or at least `yield()`) so background tasks run.
- Avoid calling fresh WiFi scans inside the connect poll loop; reuse the prior scan results.
- Guard against NULL / empty SSID or excessively long password before `WiFi.begin`.
- Channel drift issues are rarer when staying continuously in `WIFI_AP_STA`; if observed after a partial connect attempt, re-init SoftAP (defensive fallback).
- Avoid `WiFi.disconnect(true)` in tests; it turns off WiFi and will drop the AP.

### Failure Messages (Suggested Set)
- `Association timeout` (never reached connected state)
- `Authentication failed` (auth error event if available)
- `No AP found` (ESP32 scan mismatch / event)
- `Network error` (generic fallback)

Implementation note: Distinguishing between these precisely may require temporary WiFi event handlers; otherwise, use a conservative timeout vs. connected check and fall back to generic messages.

## 5. Security & Safety
- Never persist credentials during test.
- Do not echo password back.
- Wipe password staging buffer after attempt.
- Maintain AP so user doesn’t lose access mid-provisioning.

## 6. Edge Cases
| Case | Handling |
|------|----------|
| Open network (no password) | Allow; skip password length hash (still fingerprint on SSID) |
| Hidden SSID manual entry | Treat like manual path; test as normal |
| Rapid repeated clicks | Disable button while `wifiTesting` |
| Scan during test | Optionally disable scan button; safe to leave if not interfering |
| Very weak RSSI | Still allow; success message may include RSSI |
| User changes owner/timezone only | Does NOT invalidate WiFi test |
| Near watchdog threshold | Abort test at ~6.5s, return timeout message (preserves WDT margin) |
| Intermittent connect-then-drop | Treat as fail unless stable `WL_CONNECTED` for >1 poll cycle |
| Captive portal interaction | Ensure `/api/test-wifi` is allowlisted in AP captive portal and registered in AP route block |

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
0. Backend prerequisite: update AP fallback to `WIFI_AP_STA` (see Section 2.1), verify AP + scan + portal stability.
1. Firmware: add blocking `/api/test-wifi` + log lines (tag: WIFI_TEST).
1a. Captive portal: allowlist `/api/test-wifi` and register its route in the AP setup block.
2. Front-end: add store state + methods (feature flag constant optional).
3. Inject UI components in `setup.html` (button, message, banner).
4. Manual device smoke test (includes retest after intentional credential change).
5. Update `docs/frontend-patterns.md` with short entry referencing this plan.
6. (Later) Optional advisory test button on `wifi.html` (non-gated) if requested.

## 9. Success Criteria
- User cannot submit provisioning save with untested or altered WiFi credentials.
- Successful test never soft-bricks access (AP stays reachable).
- Retest requirement is always enforced on any SSID/password mutation.
- No measurable regression in initial setup load time.

## 10. Minimal Code Touch Points Summary
| Area | File(s) | Change Type |
|------|---------|-------------|
| Store | `src/data/js/stores/setup.js` | Add transient WiFi test state + methods + canSave augmentation |
| API (FE) | `src/data/js/api/setup.js` | Add `testWiFiConnection()` that POSTs `/api/test-wifi` |
| UI | `src/data/setup.html` | Insert test button, result text, banner (no structural rewrites) |
| Firmware (endpoint) | `src/web/api_system_handlers.h/.cpp` | Add `/api/test-wifi` handler (blocking) |
| Firmware (AP mode) | `src/core/network.cpp` | Change AP fallback to `WiFi.mode(WIFI_AP_STA)` |
| Firmware (routing) | `src/web/web_server.cpp` | Allowlist `/api/test-wifi` in captive portal; register route in AP block |
| Mock | `mock-server/mock-api.js` | Optional: add stub for `/api/test-wifi` for FE dev |
| Docs | `docs/frontend-patterns.md` | Add short reference section (later) |

## 11. UX Rollback Strategy
If issues arise (e.g., blocking test unreliable):
- Hide button & gating behind `window.FEATURE_WIFI_TEST` flag; default false to revert instantly.
- Endpoint remains inert when unused; no data model changes to revert.
- If AP stability issues are observed, revert AP fallback from `WIFI_AP_STA` back to `WIFI_AP` (single-line change) and disable test feature.
