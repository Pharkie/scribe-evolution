---
applyTo: "test/**/*.cpp"
---

# Unit Test Requirements

When writing unit tests for the Scribe ESP32-C3 project, follow these
guidelines:

## Test Framework

1. **PlatformIO Unity** - Use PlatformIO's built-in Unity testing framework
2. **Test Environment** - Tests run in `test` environment, separate from `main`
3. **File Naming** - Use `test_[module].cpp` naming convention

## Test Structure

1. **Setup/Teardown** - Use `setUp()` and `tearDown()` functions when needed
2. **Test Functions** - Name functions `test_[functionality]_[scenario]()`
3. **Assertions** - Use appropriate Unity assertions:
   - `TEST_ASSERT_EQUAL(expected, actual)`
   - `TEST_ASSERT_TRUE(condition)`
   - `TEST_ASSERT_NOT_NULL(pointer)`
   - `TEST_ASSERT_EQUAL_STRING(expected, actual)`

## Hardware Mocking

1. **Mock Interfaces** - Mock hardware dependencies for unit tests
2. **Serial Output** - Mock Serial interface to avoid hardware dependencies
3. **WiFi/Network** - Mock network calls for testing offline
4. **Hardware Pins** - Mock GPIO operations for LED and printer tests

## Test Categories

1. **Core Functionality Tests**:

   - Configuration parsing and validation
   - Character mapping conversions
   - Time utility functions
   - JSON helper functions

2. **Web API Tests**:

   - Request parsing and validation
   - Response formatting
   - Error handling scenarios
   - Authentication/authorization

3. **LED System Tests**:

   - Effect name validation
   - Color parsing (string to CRGB)
   - Duration handling
   - Conditional compilation (#ifdef ENABLE_LEDS)

4. **Content Generation Tests**:
   - Message formatting
   - Content type handling
   - API integration (mocked)

## Existing Test Files

1. **`test_basic.cpp`** - Basic system functionality
2. **`test_character_mapping.cpp`** - Character conversion tests
3. **`test_config_validation.cpp`** - Configuration validation
4. **`test_time_utils.cpp`** - Time and date utilities
5. **`test_web_validation.cpp`** - Web input validation

## Test Data Management

1. **Test Fixtures** - Use consistent test data across tests
2. **Edge Cases** - Test boundary conditions and error cases
3. **Valid Inputs** - Test normal operation scenarios
4. **Invalid Inputs** - Test error handling and validation

## Performance Testing

1. **Memory Usage** - Monitor heap usage during tests
2. **Timing** - Test that operations complete within expected timeframes
3. **Resource Cleanup** - Ensure proper cleanup of allocated resources

## Integration Test Patterns

1. **API Endpoint Testing** - Test full request/response cycles
2. **Configuration Roundtrips** - Test save/load configuration cycles
3. **LED Effect Chains** - Test effect transitions and cleanup
4. **Error Recovery** - Test system recovery from error states

## CI/CD Considerations

1. **Headless Testing** - All tests should run without hardware
2. **Cross-platform** - Tests should work on different development environments
3. **Fast Execution** - Keep test execution time under reasonable limits
4. **Clear Output** - Provide clear success/failure messages

## Code Coverage

1. **Critical Paths** - Ensure all critical code paths are tested
2. **Error Handling** - Test all error conditions and edge cases
3. **Public APIs** - Test all public function interfaces
4. **Configuration Scenarios** - Test different configuration combinations

## Best Practices

1. **Isolated Tests** - Each test should be independent and repeatable
2. **Clear Assertions** - Use descriptive assertion messages
3. **Minimal Setup** - Keep test setup simple and focused
4. **Documentation** - Comment complex test scenarios and expected outcomes

## Recent Testing Priorities

- Validate the new modular API handler architecture
- Test LED effect integration and conditional compilation
- Verify configuration validation improvements
- Test responsive web interface functionality
- Ensure large file detection and build process validation
