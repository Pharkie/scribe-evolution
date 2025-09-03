# Utilities - CLAUDE.md

<system_context>
Utility functions: time management, character mapping, API client.
Reusable components for consistent functionality across modules.
</system_context>

<critical_notes>

- Time utilities handle timezone conversion and formatting
- Character mapping ensures printer compatibility
- API client manages ChatGPT integration with error handling
- All utilities should be stateless where possible
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

// API client pattern
class APIClient {
bool makeRequest(const String& url, const String& payload, String& response) {
if (!WiFi.isConnected()) return false;

        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        int responseCode = http.POST(payload);
        if (responseCode == 200) {
            response = http.getString();
            return true;
        }
        return false;
    }

};
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
