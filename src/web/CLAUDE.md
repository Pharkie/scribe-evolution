# Web API - CLAUDE.md

<system_context>
HTTP server and API handlers for ESP32 web interface.
Consistent JSON responses, validation, and rate limiting.
</system_context>

<critical_notes>

- Success: HTTP 200 + empty body (no JSON parsing required)
- Error: Non-200 status + JSON error message
- All inputs MUST be validated through validation.cpp
- Rate limiting prevents DoS attacks
  </critical_notes>

<paved_path>
Response Pattern:

- POST success: `server.send(200);`
- POST error: `server.send(400, "application/json", "{\"error\":\"message\"}");`
- GET: Always return JSON with proper content-type

Handler Registration:

```cpp
void registerConfigHandlers(AsyncWebServer& server);
void registerLEDHandlers(AsyncWebServer& server);
```

</paved_path>

<patterns>
// Standard success response
void sendSuccess(AsyncWebServerRequest *request) {
    request->send(200);
}

// Standard error response  
void sendError(AsyncWebServerRequest \*request, int code, const String& message) {
String json = "{\"error\":\"" + message + "\"}";
request->send(code, "application/json", json);
}

// Validation pattern
if (!validateWiFiCredentials(ssid, password)) {
return sendError(request, 400, "Invalid WiFi credentials");
}

// Rate limiting check
if (!checkRateLimit(request->client()->remoteIP())) {
return sendError(request, 429, "Too many requests");
}

// JSON response for GET
DynamicJsonDocument doc(1024);
doc["status"] = "connected";
doc["ssid"] = currentSSID;
String response;
serializeJson(doc, response);
request->send(200, "application/json", response);
</patterns>

<common_tasks>
Adding new endpoint:

1. Create handler function with proper validation
2. Register in appropriate handler file
3. Test success and error cases
4. Verify rate limiting works
5. Check JSON buffer sizes

Testing MQTT Connections:

The `/api/test-mqtt` endpoint uses `MQTTManager::instance().testConnection()` to safely test credentials:

- Temporarily stops production MQTT (if running)
- Tests new credentials with 5-second timeout
- Feeds watchdog timer during test (prevents timeout)
- Restores production MQTT state after test
- Thread-safe via MQTTManager mutex
- Returns error message on failure for user feedback

Frontend UX Pattern:

- Button stays enabled after failure (allows retry)
- Error shown in helper text (yellow), not on button
- Button label: "Test Connection" / "Testing..." / "MQTT Connected"
- Fail-fast: test required before save when MQTT enabled
  </common_tasks>

<fatal_implications>

- Inconsistent responses = Frontend breaks randomly
- No validation = Security vulnerabilities
- Missing rate limiting = DoS attacks crash device
- Wrong buffer sizes = Memory corruption
  </fatal_implications>
