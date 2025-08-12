# Library Patches

This directory contains patch files for library modifications that suppress unnecessary warnings or fix small issues.

## webserver-304-warning-fix.patch

**Purpose**: Suppresses the "content length is zero" warning for HTTP 304 (Not Modified) and 204 (No Content) responses, which legitimately have zero content length.

**Location**: `lib/WebServer/WebServer.cpp` (line 425)

**Change**: Modified the warning condition from:
```cpp
if (content.length() == 0) {
```
to:
```cpp
if (content.length() == 0 && code != 304 && code != 204) {
```

**When to reapply**: After PlatformIO framework updates that modify the ESP32 WebServer library.

**How to reapply**:
1. Copy the original WebServer files: `cp ~/.platformio/packages/framework-arduinoespressif32/libraries/WebServer/src/WebServer.* lib/WebServer/`
2. Apply the patch: `patch lib/WebServer/WebServer.cpp < patches/webserver-304-warning-fix.patch`

**Rationale**: 304 and 204 responses are supposed to have zero content length according to HTTP specifications, so the warning is incorrect for these status codes.
