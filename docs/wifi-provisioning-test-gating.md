# WiFi Provisioning Test Gating Plan

Status: Draft (Planning Only)  
Scope: AP provisioning flow (`setup.html`) — NOT applied to STA maintenance page (`wifi.html`) in initial implementation.

## 1. Objective
Ensure first-time WiFi credentials are **validated (association + auth)** before they're persisted and the device restarts. Prevents user lock-out from mistyped SSID/password during initial provisioning.

## 2. Design Principles
- **No duplication**: Reuse existing shared scan + state logic in `wifi-utils.js` (already centralizes scanning/network processing).
- **Provisioning only gating**: Mandatory test in AP setup flow; optional (or omitted) in normal WiFi settings (STA mode) to keep that page fast and flexible.
- **Non-persistent test**: Test does *not* write to NVS; final save still goes through existing `/api/setup` POST.
- **Minimal firmware footprint**: Start with a blocking endpoint; upgrade to async + polling only if needed.
- **Deterministic invalidation**: ANY credential mutation after a successful test invalidates the pass state.

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
  - States: Idle | Testing (spinner) | Success (green check) | Fail (red text persists until change)
- Status line: displays last `wifiTestResult.message` or guidance
- Banner (small, subtle) when form valid but not yet tested: “Please test WiFi before saving.”
- Save button tooltip reasons: missing fields vs. "Test WiFi first".

No other structural changes; scanning & password UI untouched.

## 4. Firmware Endpoint
### Option A (Initial — Blocking)
`POST /api/test-wifi`
Body:
```
{ "ssid": "...", "password": "..." }
```
Process:
1. Ensure AP mode active (already in AP provisioning context).
2. Switch to `WIFI_AP_STA` if not already.
3. `WiFi.begin(ssid, password)`.
4. Poll `WiFi.status()` up to ~6–8s (e.g. 80 × 75ms with `delay(75)` or `vTaskDelay`).
5. Success if `WL_CONNECTED` reached; capture RSSI (optional), immediately `WiFi.disconnect(true)` (STA interface only) to remain on AP channel.
6. Restore `WIFI_AP` if you altered mode.
7. Return JSON:
   - 200 `{ success: true, rssi: -52 }`
   - 400 `{ success: false, message: "Association timeout" }`
8. Zero out password buffer.

### Option B (Deferred — Async)
- `POST /api/test-wifi` → 202 `{ test_id }`
- `GET /api/test-wifi/status?test_id=...` → `{ status: pending|success|fail, message }`
Adopt only if blocking variant yields watchdog or responsiveness issues.

### Failure Messages (Suggested Set)
- `Association timeout` (never reached connected state)
- `Authentication failed` (auth error event if available)
- `No AP found` (ESP32 scan mismatch / event)
- `Network error` (generic fallback)

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

## 8. Implementation Sequence (Recommended)
1. Firmware: add blocking `/api/test-wifi` + log lines (tag: WIFI_TEST).
2. Front-end: add store state + methods (feature flag constant optional).
3. Inject UI components in `setup.html` (button, message, banner).
4. Manual device smoke test.
5. Update `docs/frontend-patterns.md` with short entry referencing this plan.
6. (Later) Optional advisory test button on `wifi.html` (non-gated) if requested.

## 9. Success Criteria
- User cannot submit provisioning save with untested or altered WiFi credentials.
- Successful test never soft-bricks access (AP stays reachable).
- Retest requirement is always enforced on any SSID/password mutation.
- No measurable regression in initial setup load time.

## 10. Future Enhancements (Optional)
- Async test with progress states (associate → DHCP → success).
- RSSI + channel shown on success chip.
- Auto-scan on load: highlight previously configured SSID if present.
- Retry suggestions: channel congestion heuristics.

## 11. Deferred / Explicitly Not Doing Now
- Applying gating to STA page (`wifi.html`).
- Persisting test telemetry.
- Multi-network fallback logic.

## 12. Minimal Code Touch Points Summary
| Area | File(s) | Change Type |
|------|---------|-------------|
| Store | `src/data/js/stores/setup.js` | Add transient WiFi test state + methods + canSave augmentation |
| UI | `src/data/setup.html` | Insert test button, result text, banner (no structural rewrites) |
| Firmware | WiFi HTTP handler source | Add `/api/test-wifi` endpoint (blocking) |
| Docs | `docs/frontend-patterns.md` | Add short reference section (later) |

## 13. Rollback Strategy
If issues arise (e.g., blocking test unreliable):
- Hide button & gating behind `window.FEATURE_WIFI_TEST` flag; default false to revert instantly.
- Endpoint remains inert when unused; no data model changes to revert.

---
Prepared for implementation; no code changes applied in this document.
