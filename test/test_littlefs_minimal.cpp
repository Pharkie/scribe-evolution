#include <Arduino.h>
#include <SPIFFS.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== SPIFFS Test ===");

    // Try to mount
    if (SPIFFS.begin(false)) {
        Serial.println("✅ SPIFFS mounted");
        SPIFFS.end();
    } else {
        Serial.println("❌ Mount failed, trying format...");

        // Try to format
        if (SPIFFS.format()) {
            Serial.println("✅ Format succeeded");

            // Try mount again
            if (SPIFFS.begin(false)) {
                Serial.println("✅ Mount succeeded");
                SPIFFS.end();
            } else {
                Serial.println("❌ Mount FAILED");
            }
        } else {
            Serial.println("❌ Format FAILED");
        }
    }

    Serial.println("=== Done ===");
}

void loop() {
    delay(1000);
}
