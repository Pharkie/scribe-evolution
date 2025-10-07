# Scribe Evolution ESP32-C3 Testing Guide

## Overview

This project uses the **PlatformIO Unity Testing Framework** for embedded tests on ESP32‑C3.

## Testing Framework Status

✅ **IMPLEMENTED & WORKING**

- PlatformIO Unity testing framework integration
- ESP32-C3 test environment configuration
- Automated test execution with hardware upload
- Serial output capture and reporting
- Basic functionality validation tests

## Test Environment Setup

### Hardware Requirements

- ESP32-C3 development board
- USB connection for testing and serial output
- No additional hardware required for unit tests

### Configuration

Tests run under a dedicated PlatformIO environment (see `platformio.ini`). Example:

```ini
[env:esp32c3-test]
platform = espressif32
board = esp32-c3-devkitc-02
framework = arduino
test_framework = unity
test_port = /dev/cu.usbmodem1201
test_speed = 115200
lib_deps = ${env:esp32-c3.lib_deps}
build_flags =
    ${env:esp32-c3.build_flags}
    -DUNIT_TEST
    -DTEST_MODE
test_filter = *
build_src_filter = +<*> -<main.cpp>
```

## Current Test Suite

### `test_basic.cpp` - Core Functionality Tests

**Status: ✅ WORKING**

Tests fundamental Arduino framework functionality and patterns used throughout
the project:

- **Arduino Framework**: String handling, basic operations
- **String Operations**: Replace operations (core of character mapping)
- **Character Access**: ASCII range validation patterns
- **Configuration Constants**: Compile-time constant validation
- **Time Functions**: Arduino `millis()` and `delay()` functionality
- **Memory Functions**: Dynamic allocation testing
- **Validation Patterns**: Input validation logic testing

### Test Results

```
7 Tests 0 Failures 0 Ignored
OK
```

## Advanced Test Files (Pending Integration)

The following comprehensive test suites have been created but require source
file linking resolution:

### `test_character_mapping.cpp` - Character Translation Tests

**Status: 🔄 NEEDS LINKING RESOLUTION**

- Unicode to ASCII character mapping
- Special character cleaning
- Empty string handling
- International character support

### `test_config_validation.cpp` - Configuration Tests

**Status: 🔄 NEEDS LINKING RESOLUTION**

- Device configuration validation
- MQTT setup verification
- Printer config lookup
- Network configuration validation

### `test_web_validation.cpp` - Web Input Validation Tests

**Status: 🔄 NEEDS LINKING RESOLUTION**

- Message validation (length, content)
- JSON parsing validation
- MQTT topic validation
- Rate limiting functionality

### `test_time_utils.cpp` - Time Utility Tests

**Status: 🔄 NEEDS LINKING RESOLUTION**

- DateTime formatting
- Timezone handling
- Time constant validation

## Running Tests

### Execute All Tests

```bash
# Run complete test suite
pio test -e esp32c3-test

# Verbose output
pio test -e esp32c3-test --verbose

# Filter specific tests
pio test -e esp32c3-test --filter "*basic*"
```

### Offline vs Online Runs

- Default behavior: Tests run with WiFi + web server enabled (networked), LEDs on.
- Optional offline run: Skip network-heavy tests without creating a new env.

Use a project option to define a skip flag at invocation time:

```bash
# Skip WiFi/HTTP/network-dependent tests
pio test -e esp32c3-test -v -o "build_flags=-DTEST_SKIP_NETWORK_TESTS=1"
```

This flag:

- Skips WiFi connection and web-server route setup in the runner.
- Skips the timezone/NTP synchronization test.
- Skips endpoint tests that depend on HTTP routes.

### Quick Subsets via Filters

Run just “unit” style suites (no network):

```bash
pio test -e esp32c3-test --filter "*character_mapping*" "*config_validation*" "*web_validation*" "*time_utils*" "*nvs_config*" "*memo_handler*"
```

Run only the networked endpoint tests:

```bash
pio test -e esp32c3-test --filter "*endpoint_integration*"
```

### Automated Test Script

```bash
# Use the automated test runner
./run_tests.sh
```

The script provides:

- Colored output for easy result identification
- Automatic hardware detection
- Comprehensive error reporting
- Test summary statistics

## Test Development Guidelines

### Creating New Tests

1. **File Naming**: Use `test_[module_name].cpp` pattern
2. **Structure**: Follow Unity testing conventions
3. **Setup/Teardown**: Use `setUp()` and `tearDown()` for test isolation
4. **Assertions**: Use Unity's `TEST_ASSERT_*` macros

### Example Test Function

```cpp
void test_example_functionality() {
    // Arrange
    String input = "test input";

    // Act
    String result = processInput(input);

    // Assert
    TEST_ASSERT_EQUAL_STRING("expected output", result.c_str());
    TEST_ASSERT_TRUE(result.length() > 0);
}
```

### Unity Test Structure

```cpp
#include <unity.h>
#include <Arduino.h>

void setUp(void) {
    // Pre-test setup
}

void tearDown(void) {
    // Post-test cleanup
}

void setup() {
    delay(2000); // Wait for serial monitor
    UNITY_BEGIN();

    RUN_TEST(test_example_functionality);

    UNITY_END();
}

void loop() {
    // Unity handles everything in setup()
}
```

## Integration Notes

### Source File Linking

**Challenge**: PlatformIO test environment doesn't automatically include all
source files.

Use `build_src_filter = +<*> -<main.cpp>` to exclude the main app entry point.

**Future Enhancement**: May require individual test file organization or mock
implementations for complex dependencies.

### Hardware Dependencies

**Challenge**: Some functionality requires actual hardware (WiFi, MQTT,
printer).

**Solution**: Use dependency injection and mocking for hardware-dependent tests.

### Real-time Testing

**Challenge**: Network operations and time-dependent functionality.

**Solution**: Mock external dependencies and use controlled test data.

## Best Practices

### Test Organization

- Keep tests focused on single functionality
- Use descriptive test names
- Group related tests in logical modules
- Maintain independence between tests

### Embedded Testing Considerations

- **Memory Constraints**: Keep test data size reasonable
- **Execution Time**: Tests should complete quickly
- **Hardware Limitations**: Account for ESP32-C3 capabilities
- **Serial Output**: Use appropriate baud rates for reliable communication

### Continuous Integration

The testing framework is ready for CI/CD integration:

```yaml
# Example GitHub Actions workflow
- name: Run PlatformIO Tests
  run: pio test -e test
```

## Debugging Test Failures

### Common Issues

1. **Serial Port**: Ensure correct test port configuration
2. **Hardware Connection**: Verify ESP32-C3 board connection
3. **Memory Issues**: Check for memory leaks in tests
4. **Timing Issues**: Account for hardware boot time

### Debug Techniques

- Use `Serial.println()` for additional debug output
- Increase test timeout values if needed
- Run tests individually to isolate issues
- Check Unity documentation for assertion details

## Future Enhancements

### Planned Improvements

1. **Integration Tests**: Full system integration testing
2. **Performance Tests**: Memory usage and execution time validation
3. **Hardware-in-Loop**: Automated testing with real hardware components
4. **Coverage Analysis**: Code coverage reporting
5. **Mock Framework**: Comprehensive mocking for external dependencies

### Test Coverage Goals

- [ ] 100% core functionality coverage
- [ ] Network operation validation
- [ ] Error handling verification
- [ ] Performance benchmarking
- [ ] Security validation

## Resources

- [Unity Testing Framework](https://github.com/ThrowTheSwitch/Unity)
- [PlatformIO Testing Guide](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [ESP32-C3 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/)

---

## Quick Reference

### Test Commands

```bash
pio test -e test                    # Run all tests
pio test -e test --verbose          # Verbose output
pio test -e test --filter="basic"   # Run specific tests
./run_tests.sh                      # Automated test runner
```

### Test Status Summary

- ✅ **Basic Framework Tests**: 7/7 passing
- 🔄 **Advanced Tests**: 4 suites pending integration
- 📊 **Coverage**: Core functionality validated
- 🎯 **Next Priority**: Resolve source file linking for comprehensive testing
