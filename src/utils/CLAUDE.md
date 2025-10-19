# Utilities - CLAUDE.md

<system_context>
Utility functions: time management, character mapping, thread-safe API client.
Reusable components for consistent functionality across modules.
APIClient singleton provides thread-safe HTTP operations with mutex protection.
</system_context>

<critical_notes>

- Time utilities handle timezone conversion and formatting
- Character mapping ensures printer compatibility
- APIClient singleton (thread-safe) manages external API integration with retry/backoff
- NEVER create multiple WiFiClientSecure/HTTPClient instances - use APIClient::instance()
- All utilities except APIClient should be stateless
  </critical_notes>

<paved_path>
Key Utilities:

1. Time functions for scheduling and timestamps
2. Character mapping for thermal printer compatibility
3. API client for external service integration
4. String utilities for safe manipulation
   </paved_path>

<patterns>
// Time utilities
String getCurrentTimestamp() {
    return getFormattedTime("%Y-%m-%d %H:%M:%S");
}

bool isInTimeWindow(int startHour, int endHour) {
int currentHour = getCurrentHour();
return (currentHour >= startHour && currentHour < endHour);
}

// Character mapping for printer
String mapSpecialCharacters(const String& input) {
String result = input;
result.replace("'", "'"); // Smart quote to regular
result.replace(""", "\""); // Smart quote to regular
return result;
}

// Thread-safe API client - singleton pattern with mutex
// Usage: APIClient::instance().fetchFromAPI(url, userAgent, timeout)
String response = APIClient::instance().fetchFromAPI(
"https://api.example.com/data",
"ScribeEvolution/1.0",
5000 // timeout in ms
);

// POST with Bearer auth
String result = APIClient::instance().postToAPIWithBearer(
"https://api.example.com/endpoint",
bearerToken,
jsonPayload,
"ScribeEvolution/1.0"
);

// Backward-compatible wrapper functions also available:
String data = fetchFromAPI(url, userAgent, timeout);
</patterns>

<common_tasks>
Adding new utility:

1. Create stateless function where possible
2. Add appropriate error handling
3. Document parameters and return values
4. Add to relevant header file
5. Test edge cases
   </common_tasks>

<fatal_implications>

- Stateful utilities = Threading issues
- No error handling = Silent failures cascade
- Memory leaks in utilities = System-wide instability
- Wrong character mapping = Printer corruption
  </fatal_implications>
