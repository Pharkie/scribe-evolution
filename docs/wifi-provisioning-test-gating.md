# WiFi Provisioning Test Gating

Short overview of the AP‑mode WiFi credential test flow used during first‑time setup.

## Objective

Validate SSID/password (association + auth) before persisting to NVS and rebooting, preventing lock‑out on mistyped credentials.

## Behavior

- AP fallback runs `WIFI_AP_STA` during setup; STA is used for scan + test while AP serves the captive portal.
- Setup page requires a successful “Test WiFi” run before enabling Save.
- Any change to SSID/password invalidates the previous successful test.

## Endpoint

- `POST /api/test-wifi` (AP mode)
- Body: `{ "ssid": "...", "password": "..." }`
- Blocking test (≈6–7s max): attempts connection, then disconnects STA (AP remains reachable).
- Responses: `200 { success:true, rssi }`, `408 { success:false, message }`, `409 { success:false, message:"Test already running" }`, `422 { success:false, message:"Invalid payload" }`.

## Frontend Notes

- Transient state: `wifiTesting`, `wifiTestResult`, `wifiTestPassed`, `dirtySinceLastTest`.
- UI: button below password; Save gated on `wifiTestPassed && !dirtySinceLastTest`.

## Safety

- No credentials are persisted during test.
- Passwords are never echoed back.
- Watchdog‑safe delays; AP remains reachable.
