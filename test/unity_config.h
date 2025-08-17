/**
 * @file unity_config.h
 * @brief Unity test framework configuration for PlatformIO
 */

#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Unity Framework Configuration
#define UNITY_INCLUDE_DOUBLE
#define UNITY_INCLUDE_FLOAT
#define UNITY_INCLUDE_64
#define UNITY_INCLUDE_PRINT_FORMATTED

// Output configuration for VS Code Test Explorer
#define UNITY_OUTPUT_COLOR
#define UNITY_OUTPUT_START() Serial.begin(115200)
#define UNITY_OUTPUT_CHAR(c) Serial.print((char)(c))
#define UNITY_OUTPUT_FLUSH() Serial.flush()
#define UNITY_OUTPUT_COMPLETE() Serial.println()

// Test discovery markers for VS Code
#define UNITY_BEGIN_TEST() Serial.println("UNITY_TEST_BEGIN")
#define UNITY_END_TEST() Serial.println("UNITY_TEST_END")

// Test result markers for test discovery
#define UNITY_PRINT_TEST_RESULTS()                                            \
    do                                                                        \
    {                                                                         \
        Serial.printf("UNITY_TEST_RESULTS: %d tests, %d passed, %d failed\n", \
                      Unity.NumberOfTests,                                    \
                      Unity.NumberOfTests - Unity.TestFailures,               \
                      Unity.TestFailures);                                    \
    } while (0)

#endif // UNITY_CONFIG_H
