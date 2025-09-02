# Unit Test Requirements

## Test Framework

- Use PlatformIO Unity testing framework
- Tests run in `test` environment, separate from `main`
- Use `test_[module].cpp` naming convention

## Test Structure

- Use `setUp()` and `tearDown()` functions when needed
- Name functions `test_[functionality]_[scenario]()`
- Use appropriate Unity assertions: `TEST_ASSERT_EQUAL`, `TEST_ASSERT_TRUE`, `TEST_ASSERT_NOT_NULL`, `TEST_ASSERT_EQUAL_STRING`

## Hardware Mocking

- Mock hardware dependencies for unit tests
- Mock Serial interface to avoid hardware dependencies
- Mock network calls for testing offline
- Mock GPIO operations for LED and printer tests

## Test Categories

**Core Functionality Tests:**

- Configuration parsing and validation
- Character mapping conversions
- Time utility functions
- JSON helper functions

**Web API Tests:**

- Request parsing and validation
- Response formatting
- Error handling scenarios

**LED System Tests:**

- Effect name validation
- Color parsing (string to CRGB)
- Duration handling
- Conditional compilation (#ifdef ENABLE_LEDS)

## Existing Test Files

- `test_basic.cpp` - Basic system functionality
- `test_character_mapping.cpp` - Character conversion tests
- `test_config_validation.cpp` - Configuration validation
- `test_time_utils.cpp` - Time and date utilities
- `test_web_validation.cpp` - Web input validation

## Best Practices

- Each test should be independent and repeatable
- Use descriptive assertion messages
- Keep test setup simple and focused
- Test all critical code paths and error conditions
- Ensure tests run without hardware (headless testing)
- Monitor heap usage during tests
- Test full request/response cycles for API endpoints
