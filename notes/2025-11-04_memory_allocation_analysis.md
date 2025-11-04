# Memory Allocation Analysis: Heap Fragmentation Vulnerabilities

**Date**: 2025-11-04
**Analysis Type**: Read-only codebase exploration
**Focus**: Large memory allocations (>4KB) and heap fragmentation risks

## Executive Summary

This analysis identifies all locations in the Scribe Evolution codebase where large memory allocations occur that could be vulnerable to heap fragmentation issues. The codebase already implements heap fragmentation checks in **one critical location** (FastLED initialization), but several other areas allocate large buffers without checking for contiguous memory availability.

**Key Finding**: Only **LedEffects.cpp** properly checks `ESP.getMaxAllocHeap()` before large allocations. Several other components allocate memory >4KB without fragmentation checks.

---

## Critical Pattern: Heap Fragmentation Check

### The Correct Pattern (from LedEffects.cpp)

```cpp
// Check heap availability BEFORE FastLED allocation
const size_t fastledBufferSize = count * 96;
const size_t safetyMargin = 10240; // 10KB for effects + HTTP
const size_t minHeapRequired = fastledBufferSize + safetyMargin;

size_t freeHeap = ESP.getFreeHeap();
size_t maxContiguousBlock = ESP.getMaxAllocHeap();  // ← CRITICAL CHECK

// Check 1: Contiguous block size (prevents fragmentation failures)
if (maxContiguousBlock < fastledBufferSize) {
    LOG_ERROR("LEDS", "Heap too fragmented for %d LEDs!", count);
    LOG_ERROR("LEDS", "Total free: %d bytes, largest block: %d bytes, need: %d bytes",
              freeHeap, maxContiguousBlock, fastledBufferSize);
    return false;
}

// Check 2: Total free heap (prevents exhaustion)
if (freeHeap < minHeapRequired) {
    LOG_ERROR("LEDS", "Insufficient total heap for %d LEDs: %d bytes free, need %d bytes",
              count, freeHeap, minHeapRequired);
    return false;
}
```

**Why this matters**: `ESP.getFreeHeap()` only reports total free memory, which may be scattered across many small fragments. `ESP.getMaxAllocHeap()` reports the **largest contiguous block**, which is what actually matters for large allocations.

**Real-world example**: At 130 LEDs (12.5KB RMT buffer needed):

- Total free heap: 198KB ✓ (misleading - looked fine!)
- Largest contiguous block: ~11KB ✗ (actual problem - too fragmented!)
- Result: FastLED allocation failed with heap corruption

---

## Findings by Category

### 1. FastLED RMT Buffers (LED System)

#### 1.1 LedEffects Main Initialization ✅ **PROPERLY PROTECTED**

**Location**: `/src/leds/LedEffects.cpp:159-192`

**Allocation Size**: `ledCount * 96` bytes (RMT buffer)

- 100 LEDs = 9.6KB
- 130 LEDs = 12.5KB
- 300 LEDs = 28.8KB

**Heap Check**: ✅ **EXEMPLARY IMPLEMENTATION**

```cpp
size_t maxContiguousBlock = ESP.getMaxAllocHeap();
if (maxContiguousBlock < fastledBufferSize) {
    // Proper error with diagnostic info
}
```

**Risk Level**: ✅ **LOW** (already protected)

**Notes**: This is the gold standard implementation. Uses dual checks (contiguous + total) with detailed error messages showing actual vs required memory.

---

#### 1.2 StatusLed Module (Custom PCB Only)

**Location**: `/src/leds/StatusLed.h:54`, `/src/leds/StatusLed.cpp`

**Allocation Size**: `1 LED * 96 bytes = 96 bytes`

**Heap Check**: ⚠️ Not checked (but allocation is tiny)

**Risk Level**: ✅ **NEGLIGIBLE** (allocation <100 bytes, well below fragmentation threshold)

**Notes**: Single LED for status indicator. No check needed due to tiny size.

---

### 2. Dynamic Memory Allocations

#### 2.1 LogManager Message Buffers

**Location**: `/src/core/LogManager.cpp:62`, `LogManager.cpp:152`

**Allocation Pattern**:

```cpp
char* line = (char*)malloc(maxLineLen);  // Default: 512 bytes
```

**Allocation Size**: 512 bytes (default `maxLineLen`)

**Heap Check**: ❌ None (silently drops message on allocation failure)

**Risk Level**: ✅ **LOW**

- Allocation is small (<1KB)
- Graceful degradation (drops message vs crashing)
- Called frequently, so failures would be noticed immediately

**Notes**: Intentionally designed to fail gracefully. Small allocation size means fragmentation unlikely to be an issue.

---

#### 2.2 APIClient - WiFiClientSecure & HTTPClient

**Location**: `/src/utils/api_client.cpp:55-66`

**Allocation Pattern**:

```cpp
wifiClient = new WiFiClientSecure();  // Size: ~1-2KB (internal buffers)
httpClient = new HTTPClient();        // Size: <1KB
```

**Heap Check**: ❌ Checks if allocation returned nullptr, but doesn't pre-check heap

**Risk Level**: ⚠️ **MEDIUM**

**Potential Issue**:

- WiFiClientSecure allocates SSL buffers internally (mbedTLS)
- SSL/TLS operations can require **16-32KB contiguous memory** for handshake buffers
- No pre-check before allocation
- Failure logged but system continues (may cause cascading failures)

**Recommendation**:

```cpp
// Before allocation
size_t sslBufferNeeded = 32768; // 32KB for SSL handshake
if (ESP.getMaxAllocHeap() < sslBufferNeeded) {
    LOG_ERROR("API", "Insufficient heap for SSL: %d bytes available, need %d",
              ESP.getMaxAllocHeap(), sslBufferNeeded);
    return;
}
wifiClient = new WiFiClientSecure();
```

**Files Affected**:

- `/src/utils/api_client.cpp:55-66` (initialization)
- All code using `APIClient::instance().fetchFromAPI()` (indirect)

---

#### 2.3 LED Effect Dynamic Arrays

**Location**: `/src/leds/effects/Matrix.cpp:32`, `/src/leds/effects/TwinkleStars.cpp:32`

**Allocation Pattern**:

```cpp
// Matrix effect
matrixDrops = new MatrixDrop[config.drops];  // Typically 10-20 drops
// Size: sizeof(MatrixDrop) * drops ≈ 16 bytes * 20 = 320 bytes

// Twinkle effect
twinkleStars = new TwinkleState[config.density];  // Typically 10-30 stars
// Size: sizeof(TwinkleState) * density ≈ 12 bytes * 30 = 360 bytes
```

**Heap Check**: ❌ None (allocates, checks for nullptr, deallocates on failure)

**Risk Level**: ✅ **LOW**

- Allocations are small (<500 bytes typical)
- Cleaned up in deallocate methods
- Effects are short-lived

**Notes**: These are transient allocations for effect state. Small size makes fragmentation unlikely.

---

### 3. JSON Document Allocations

#### 3.1 Dynamic JSON Documents (ArduinoJson 7)

**Locations**: Multiple files (see grep results for `JsonDocument`)

**Allocation Pattern**:

```cpp
JsonDocument doc;  // ArduinoJson 7 auto-sizing
// OR
std::unique_ptr<JsonDocument> doc(new JsonDocument());  // Heap-allocated
```

**Allocation Size**: Varies by content (typically 512B - 4KB)

**Heap Check**: ❌ None (relies on ArduinoJson's internal handling)

**Risk Level**: ✅ **LOW**

**Notes**:

- ArduinoJson 7 uses dynamic sizing with internal fragmentation handling
- Most JSON docs in codebase are <2KB
- Library handles allocation failures gracefully
- One exception: `/src/web/api_system_handlers.cpp:199` uses heap allocation for large route listing

**Files Using JsonDocument**:

- `/src/utils/json_helpers.cpp` (error responses)
- `/src/content/content_handlers.cpp` (API payloads)
- `/src/content/content_generators.cpp` (ChatGPT payloads)
- `/src/web/web_server.cpp` (file system listings)
- `/src/web/api_system_handlers.cpp` (system info, **large allocation at line 199**)
- `/src/web/validation.cpp` (validation responses)
- `/src/web/api_nvs_handlers.cpp:24` (NVS dump, **heap-allocated**)
- `/src/web/api_memo_handlers.cpp` (memo operations)
- `/src/core/mqtt_handler.cpp` (MQTT message parsing)

---

#### 3.2 Large JSON Documents (Heap-Allocated)

**Location**: `/src/web/api_system_handlers.cpp:199`, `/src/web/api_nvs_handlers.cpp:24`

**Allocation Pattern**:

```cpp
std::unique_ptr<JsonDocument> doc(new JsonDocument());
```

**Potential Size**: Could exceed 4KB for:

- `/api/routes` endpoint (full route listing)
- `/api/nvs-dump` endpoint (all NVS keys + values)

**Heap Check**: ❌ None

**Risk Level**: ⚠️ **MEDIUM**

**Recommendation**: Add heap check before large JSON operations:

```cpp
if (ESP.getMaxAllocHeap() < 8192) {  // 8KB threshold
    LOG_WARNING("WEB", "Low memory for JSON response: %d bytes available",
                ESP.getMaxAllocHeap());
    sendErrorResponse(request, 507, "Insufficient memory");
    return;
}
std::unique_ptr<JsonDocument> doc(new JsonDocument());
```

---

### 4. File I/O Operations

#### 4.1 File Reading (LittleFS)

**Locations**:

- `/src/content/content_generators.cpp:39` (riddles.ndjson)
- `/src/core/mqtt_handler.cpp:209,271` (CA certificate)
- Test files (various)

**Allocation Pattern**:

```cpp
File file = LittleFS.open("/resources/riddles.ndjson", "r");
String line = file.readStringUntil('\n');  // Line-by-line
// OR
String certContent = certFile.readString();  // Entire file
```

**Heap Check**: ❌ None

**Risk Level**: ⚠️ **MEDIUM to HIGH** (depends on file size)

**Analysis**:

**Low Risk** (line-by-line reading):

- `/src/content/content_generators.cpp:53` - Reads riddles line-by-line
- Each line <1KB typically
- No large contiguous allocation

**Higher Risk** (whole-file reading):

- `/src/core/mqtt_handler.cpp:215,277` - Reads entire CA certificate
- Certificate file: ~1.5KB (PEM format)
- Still below fragmentation threshold but worth monitoring

**Recommendation for large files**:

```cpp
File certFile = LittleFS.open("/resources/isrg-root-x1.pem", "r");
if (!certFile) { /* error */ }

size_t fileSize = certFile.size();
if (fileSize > 4096 && ESP.getMaxAllocHeap() < fileSize + 2048) {
    LOG_ERROR("MQTT", "Insufficient heap for certificate: need %d bytes, have %d",
              fileSize, ESP.getMaxAllocHeap());
    certFile.close();
    return;
}

String certContent = certFile.readString();
```

**Files Affected**:

- `/src/content/content_generators.cpp:39-53` (riddles - **safe, line-by-line**)
- `/src/core/mqtt_handler.cpp:209-215` (TLS cert - **1.5KB, borderline**)
- `/src/core/mqtt_handler.cpp:271-277` (TLS cert duplicate - **1.5KB, borderline**)

---

### 5. Network Buffer Operations

#### 5.1 AsyncWebServer Response Buffers

**Locations**:

- `/src/web/web_server.cpp:177` (index.html)
- `/src/web/web_handlers.cpp:78,86` (404 pages)
- `/src/web/api_config_handlers.cpp:915,992` (config responses)
- `/src/web/api_system_handlers.cpp:209` (system info)

**Allocation Pattern**:

```cpp
AsyncWebServerResponse* response = request->beginResponse(
    LittleFS, "/index.html", "text/html"
);
// OR
AsyncWebServerResponse *res = request->beginResponse(
    200, "application/json", responseStr
);
```

**Heap Check**: ❌ None (handled internally by ESPAsyncWebServer)

**Risk Level**: ⚠️ **MEDIUM**

**Analysis**:

- ESPAsyncWebServer uses chunked responses (internal buffering)
- Typical chunk size: 1460 bytes (TCP MSS)
- File responses stream from LittleFS (no large buffer needed)
- String responses may allocate for entire payload

**Potential Issue**:

- Large JSON responses (e.g., `/api/routes` with 50+ endpoints)
- Could exceed 4KB for response string
- AsyncWebServer will try to send in chunks, but initial String allocation happens first

**Recommendation**:

```cpp
// Before serializing large JSON to String
size_t estimatedSize = measureJsonPretty(doc);  // ArduinoJson helper
if (estimatedSize > 4096 && ESP.getMaxAllocHeap() < estimatedSize + 2048) {
    sendErrorResponse(request, 507, "Response too large for available memory");
    return;
}
String response;
serializeJson(doc, response);
```

---

#### 5.2 MQTT Payloads

**Locations**: `/src/core/mqtt_handler.cpp` (various)

**Allocation Pattern**:

```cpp
JsonDocument payloadDoc;
payloadDoc["header"] = header;
payloadDoc["body"] = body;  // Can be large (memo content, ChatGPT response)
payloadDoc["timestamp"] = getFormattedDateTime();
String payload;
serializeJson(payloadDoc, payload);
```

**Buffer Size**: Configured at compile time

```cpp
const int mqttBufferSize = 4096;  // From system_constants.h
```

**Heap Check**: ❌ None

**Risk Level**: ⚠️ **MEDIUM**

**Analysis**:

- MQTT payload limit: 4096 bytes
- Memos can be up to 500 characters (~500 bytes)
- ChatGPT responses: max 150 tokens (~600 bytes)
- Header + body + JSON overhead: typically <2KB
- BUT: No check before allocating 4KB buffer

**Potential Issue**:

- Multiple MQTT operations in quick succession
- Each allocates a 4KB JsonDocument
- Could fragment heap if many large payloads sent rapidly

**Recommendation**:

```cpp
// In publishMessage() or critical MQTT operations
if (ESP.getMaxAllocHeap() < 6144) {  // 4KB payload + 2KB overhead
    LOG_WARNING("MQTT", "Low heap for MQTT publish: %d bytes available",
                ESP.getMaxAllocHeap());
    return false;
}
JsonDocument payloadDoc;
// ... rest of code
```

---

### 6. String Concatenation and Manipulation

#### 6.1 Arduino String Operations

**Search Results**: No large String concatenations found (intentionally avoided in codebase)

**Risk Level**: ✅ **LOW**

**Notes**:

- Codebase generally avoids repeated String concatenation
- Most String operations are single assignments or small appends
- No evidence of pathological String fragmentation patterns

---

### 7. Static Arrays (Compile-Time Allocation)

#### 7.1 LED Array (Static Global)

**Location**: `/src/leds/LedEffects.cpp:24`

```cpp
#define MAX_LEDS 300
CRGB staticLEDs[MAX_LEDS];  // 300 LEDs * 3 bytes RGB = 900 bytes
```

**Allocation Size**: 900 bytes (static data segment)

**Heap Check**: ⚠️ N/A (not heap-allocated, but consumes RAM)

**Risk Level**: ✅ **LOW** (static allocation, not subject to fragmentation)

**Notes**:

- Allocated at compile time in .bss section
- Does not fragment heap
- Always available (not dependent on runtime heap state)
- Intentionally moved from heap to static to avoid ESP32-C3 crashes

---

#### 7.2 Test/Debug Arrays

**Locations**:

- `/test_main.cpp:56` - `CRGB leds[NUM_LEDS]`
- `/test/test_c3_fastled.cpp:47` - `CRGB leds[NUM_LEDS]`

**Heap Check**: N/A (test code, static allocation)

**Risk Level**: ✅ **LOW** (test environments only)

---

## Summary of Findings by Risk Level

### HIGH RISK (Requires Immediate Attention)

**None identified** - No allocations >16KB without any error checking.

### MEDIUM RISK (Should Add Heap Checks)

1. **APIClient - WiFiClientSecure SSL Buffers**
   - Location: `/src/utils/api_client.cpp:55-66`
   - Size: 16-32KB (mbedTLS internal buffers)
   - Recommendation: Pre-check heap before SSL client creation
   - Priority: **HIGH** (SSL failures cascade to all API operations)

2. **Large JSON Documents (Heap-Allocated)**
   - Locations: `/src/web/api_system_handlers.cpp:199`, `/src/web/api_nvs_handlers.cpp:24`
   - Size: Potentially 4-8KB
   - Recommendation: Check heap before creating large JSON responses
   - Priority: **MEDIUM** (rare operations, but user-facing)

3. **MQTT Payload Buffers**
   - Location: `/src/core/mqtt_handler.cpp` (various)
   - Size: Up to 4KB
   - Recommendation: Check heap before large MQTT operations
   - Priority: **MEDIUM** (frequent operations, but size-limited)

4. **File I/O (Whole-File Reads)**
   - Locations: `/src/core/mqtt_handler.cpp:215,277` (CA certificate)
   - Size: ~1.5KB
   - Recommendation: Check heap before reading large files
   - Priority: **LOW-MEDIUM** (infrequent, but critical for MQTT)

5. **AsyncWebServer Large Responses**
   - Locations: `/src/web/api_system_handlers.cpp:209` (route listing)
   - Size: Variable (could exceed 4KB)
   - Recommendation: Estimate JSON size before serialization
   - Priority: **LOW** (admin endpoint, infrequent)

### LOW RISK (No Action Needed)

1. **LogManager Message Buffers** (512 bytes, graceful failure)
2. **LED Effect State Arrays** (<500 bytes, transient)
3. **Standard JSON Documents** (<2KB, library-handled)
4. **StatusLed Buffer** (96 bytes, negligible)
5. **Line-by-Line File Reading** (<1KB per line)
6. **Static LED Array** (900 bytes, not heap-allocated)

---

## Recommended Implementation Priority

### Phase 1: Critical SSL Buffers (Immediate)

**Target**: `/src/utils/api_client.cpp`

Add heap check before WiFiClientSecure creation:

```cpp
void APIClient::begin() {
    // ... existing code ...

    // Check heap before SSL client (requires 32KB contiguous for handshake)
    const size_t sslMinHeap = 32768;
    if (ESP.getMaxAllocHeap() < sslMinHeap) {
        LOG_ERROR("API", "Insufficient contiguous heap for SSL client");
        LOG_ERROR("API", "Need %d bytes, have %d bytes (total free: %d)",
                  sslMinHeap, ESP.getMaxAllocHeap(), ESP.getFreeHeap());
        return;
    }

    wifiClient = new WiFiClientSecure();
    // ... rest of code ...
}
```

**Impact**: Prevents cryptic SSL failures that cascade to all API operations (ChatGPT, jokes, trivia, news).

---

### Phase 2: MQTT Buffer Allocations (High Priority)

**Target**: `/src/core/mqtt_handler.cpp`

Add checks before publishing large messages:

```cpp
bool MQTTManager::publishMessage(const String& topic, const String& header, const String& body) {
    // ... existing validation ...

    // Check heap before large JSON allocation
    const size_t mqttJsonSize = 4096 + 2048;  // Buffer + overhead
    if (ESP.getMaxAllocHeap() < mqttJsonSize) {
        LOG_WARNING("MQTT", "Insufficient heap for MQTT publish");
        LOG_WARNING("MQTT", "Need %d bytes, have %d bytes",
                    mqttJsonSize, ESP.getMaxAllocHeap());
        return false;
    }

    JsonDocument payloadDoc;
    // ... rest of code ...
}
```

**Impact**: Prevents MQTT publish failures during low-memory conditions.

---

### Phase 3: Large JSON Responses (Medium Priority)

**Target**: `/src/web/api_system_handlers.cpp`, `/src/web/api_nvs_handlers.cpp`

Add checks before heap-allocated JsonDocuments:

```cpp
void handleRoutes(AsyncWebServerRequest *request) {
    // Check heap before large JSON
    const size_t minHeapForRoutes = 8192;
    if (ESP.getMaxAllocHeap() < minHeapForRoutes) {
        LOG_WARNING("WEB", "Low memory for routes listing");
        sendErrorResponse(request, 507, "Insufficient memory for response");
        return;
    }

    std::unique_ptr<JsonDocument> doc(new JsonDocument());
    // ... rest of code ...
}
```

**Impact**: Graceful degradation instead of crashes on admin endpoints.

---

### Phase 4: File I/O Operations (Low Priority)

**Target**: `/src/core/mqtt_handler.cpp` (certificate loading)

Add checks before reading large files:

```cpp
// In setupMQTTInternal()
File certFile = LittleFS.open("/resources/isrg-root-x1.pem", "r");
if (!certFile) {
    LOG_ERROR("MQTT", "Failed to open CA certificate file");
    return;
}

size_t certSize = certFile.size();
if (certSize > 2048 && ESP.getMaxAllocHeap() < certSize + 2048) {
    LOG_ERROR("MQTT", "Insufficient heap for certificate: need %d, have %d",
              certSize, ESP.getMaxAllocHeap());
    certFile.close();
    return;
}

String certContent = certFile.readString();
```

**Impact**: Prevents rare MQTT TLS setup failures.

---

## Reference: ESP32 Memory Architecture

### ESP32-C3 (Single-Core, 400KB RAM)

- Total RAM: ~400KB
- Available heap (typical): 170-230KB after boot
- Heap fragmentation risk threshold: **>4KB allocations**

### ESP32-S3 (Dual-Core, ~512KB RAM)

- Total RAM: ~512KB
- Available heap (typical): 240-300KB after boot
- Heap fragmentation risk threshold: **>4KB allocations**

### ESP32-S3R8 (with 2MB PSRAM)

- Total RAM: ~512KB + 2MB PSRAM
- PSRAM accessible via heap allocator
- Less fragmentation concern (larger address space)
- **Note**: Current builds do not use PSRAM in this codebase

---

## Files Requiring Attention

### Critical (Add Heap Checks Immediately)

1. `/src/utils/api_client.cpp:55-66` - WiFiClientSecure allocation

### Important (Add Heap Checks Soon)

2. `/src/core/mqtt_handler.cpp:599` - MQTT publish payloads
3. `/src/core/mqtt_handler.cpp:215,277` - CA certificate loading

### Recommended (Add When Time Permits)

4. `/src/web/api_system_handlers.cpp:199` - Routes listing
5. `/src/web/api_nvs_handlers.cpp:24` - NVS dump

---

## Conclusion

The Scribe Evolution codebase demonstrates **excellent heap fragmentation awareness** in its most critical component (FastLED initialization). However, several other large allocations lack similar protection:

**Key Recommendations**:

1. Apply the LedEffects.cpp heap check pattern to WiFiClientSecure initialization
2. Add heap checks before MQTT payload allocations
3. Consider heap checks for large JSON responses
4. Monitor file I/O operations for large files

**Overall Code Quality**: The codebase is well-structured with minimal fragmentation-prone patterns. Most allocations are small, transient, or handled by libraries with internal fragmentation management. The main gaps are in network stack components (SSL, MQTT) where large contiguous buffers are required.

---

## Additional Notes

### Why ESP.getMaxAllocHeap() Is Critical

Standard `ESP.getFreeHeap()` can be **misleading** in fragmented conditions:

| Scenario   | getFreeHeap() | getMaxAllocHeap() | 12.5KB Allocation Result |
| ---------- | ------------- | ----------------- | ------------------------ |
| Fresh boot | 220KB         | 160KB             | ✅ Success               |
| After use  | 198KB         | 11KB              | ❌ Failure (fragmented!) |

**Always check contiguous block size** for allocations >4KB.

### Related Documentation

- `/notes/2025-11-04_fastled_memory_analysis.md` - FastLED heap fix details
- `/src/leds/CLAUDE.md:344-392` - Heap fragmentation pattern documentation
- `/src/core/CLAUDE.md` - Memory architecture overview

---

**Analysis Complete**: 2025-11-04
**Findings Documented For**: Main agent review and implementation planning
