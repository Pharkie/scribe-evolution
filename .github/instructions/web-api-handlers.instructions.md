# Web API Handler Requirements

## Code Organization

- Keep handlers focused on single responsibilities
- Maintain files under 800 lines
- Use `handle[Function]` naming pattern (e.g., `handleConfigGet`, `handleLedEffect`)

## Response Patterns

- Import from `json_helpers.h`: `sendErrorResponse()`, `sendSuccessResponse()`, `sendValidationError()`
- Always include `success` boolean and descriptive messages
- Use appropriate HTTP status codes (400 validation, 429 rate limiting, 500 server errors)

## Input Validation

- Check request method first: `if (request->method() != HTTP_POST)`
- Use `DynamicJsonDocument` with appropriate buffer sizes
- Validate all inputs before processing
- Use validation.h functions for consistent error responses

## Async Patterns

- Use `request->send()` for responses
- Use `request->hasParam()` and `request->getParam()` for parameter access
- Handle JSON body via `request->getParam("body", true)->value()`

## LED Handler Specifics

- Wrap LED code in `#ifdef ENABLE_LEDS`
- Support color names (red, green, blue, etc.) to CRGB conversion
- Default durations: 3s for saves, 10s for testing
- Support effects: simple_chase, rainbow, twinkle, chase, pulse, matrix

## Configuration Handler Specifics

- Check all required fields for each section
- Never expose sensitive data in error messages
- Update configuration atomically or rollback on failure
- Include device info like firmware version, boot time

## Error Handling Best Practices

- Use LOG_ERROR, LOG_NOTICE, LOG_VERBOSE appropriately
- Provide clear, actionable error messages
- Gracefully handle missing optional parameters
- Don't leak internal implementation details in errors

## Performance Considerations

- Use appropriate buffer sizes for JSON documents
- Prefer Arduino String class over char arrays
- Respond immediately, use background tasks for long operations
- Ensure proper cleanup in error paths
