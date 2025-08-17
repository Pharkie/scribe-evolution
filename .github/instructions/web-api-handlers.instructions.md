---
applyTo: "src/web/**/*.cpp"
---

# Web API Handler Requirements

When working with web API handlers, follow these specific guidelines for the
ESP32-C3 Scribe project:

## Code Organization

1. **Modular Architecture** - Keep handlers focused on single responsibilities
2. **File Size Limit** - Maintain files under 800 lines (current success: broke
   1009-line monolith into 4 modules)
3. **Consistent Naming** - Use `handle[Function]` naming pattern (e.g.,
   `handleConfigGet`, `handleLedEffect`)

## Response Patterns

1. **Use Shared Utilities** - Import from `json_helpers.h` for response
   functions:
   - `sendErrorResponse(request, code, message)`
   - `sendSuccessResponse(request, message)`
   - `sendValidationError(request, result)` (from validation.h)
2. **JSON Structure** - Always include `success` boolean and descriptive
   messages
3. **HTTP Status Codes** - Use appropriate codes (400 for validation, 429 for
   rate limiting, 500 for server errors)

## Input Validation

1. **Method Validation** - Check request method first:
   `if (request->method() != HTTP_POST)`
2. **JSON Parsing** - Use `DynamicJsonDocument` with appropriate buffer sizes
3. **Parameter Validation** - Validate all inputs before processing
4. **Error Handling** - Use validation.h functions for consistent error
   responses

## Async Patterns

1. **Non-blocking Operations** - Use `request->send()` for responses
2. **Parameter Access** - Use `request->hasParam()` and `request->getParam()`
3. **Body Parsing** - Handle JSON body via
   `request->getParam("body", true)->value()`

## LED Handler Specifics

1. **Conditional Compilation** - Wrap LED code in `#ifdef ENABLE_LEDS`
2. **Color Parsing** - Support color names (red, green, blue, etc.) to CRGB
   conversion
3. **Duration Handling** - Default to reasonable durations (3s for saves, 10s
   for testing)
4. **Effect Names** - Support: simple_chase, rainbow, twinkle, chase, pulse,
   matrix

## Configuration Handler Specifics

1. **Comprehensive Validation** - Check all required fields for each section
2. **Security** - Never expose sensitive data in error messages
3. **Atomic Updates** - Update configuration atomically or rollback on failure
4. **Runtime Info** - Include device info like firmware version, boot time

## Error Handling Best Practices

1. **Logging** - Use LOG_ERROR, LOG_NOTICE, LOG_VERBOSE appropriately
2. **User-Friendly Messages** - Provide clear, actionable error messages
3. **Fallback Behavior** - Gracefully handle missing optional parameters
4. **Security** - Don't leak internal implementation details in errors

## Performance Considerations

1. **Memory Management** - Use appropriate buffer sizes for JSON documents
2. **String Handling** - Prefer Arduino String class over char arrays
3. **Quick Responses** - Respond immediately, use background tasks for long
   operations
4. **Resource Cleanup** - Ensure proper cleanup in error paths

## Recent Patterns to Follow

- Modular file structure (system, config, LED handlers separated)
- Shared utility functions via json_helpers.h
- Consistent validation patterns via validation.h
- Green LED success feedback for settings saves
- Toast message integration for user feedback
