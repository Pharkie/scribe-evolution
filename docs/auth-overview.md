# Authentication & CSRF

Summary of the authentication model for the web API.

## Sessions

- On visiting `/`, the server creates a session and sets a cookie (`scribe_session`).
- Cookies: `HttpOnly; SameSite=Strict; Path=/; Max-Age=<hours>`.
- Sessions expire after inactivity; activity refreshes the timer.

## Access Control

- AP mode (setup): setup endpoints are public.
- STA mode: all `/api/*` endpoints require authentication, including `/api/routes` and `/api/timezones`.

## CSRF Protection

- Required for state‑changing requests (POST/PUT/DELETE) in STA mode.
- Client sends `X-CSRF-Token` header (UI reads from `scribe_csrf` cookie and sends it).

## Frontend Behavior

- If a session is missing/expired, API calls may return 401; the UI reloads to obtain a new session.
- CSRF is handled automatically by the UI; no user action required.
