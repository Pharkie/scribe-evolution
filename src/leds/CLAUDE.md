# LED Effects - CLAUDE.md

<system_context>
LED effects system with conditional compilation.
FastLED-based cycle management and effect registry.
</system_context>

<critical_notes>

- Conditional compilation with #ifdef ENABLE_LEDS
- All LED GPIO pins must pass safety validation
- Effects use cycle-based timing for consistency
- Effect registry pattern allows extensibility
  </critical_notes>

<paved_path>
Effect Development:

1. Create effect class inheriting from base effect
2. Implement update() method with cycle timing
3. Register effect in effect registry
4. Test with hardware validation

Available Effects:
ChaseSingle, ChaseMulti, Rainbow, Twinkle, Pulse, Matrix
</paved_path>

<patterns>
// Conditional compilation guard
#ifdef ENABLE_LEDS

// Effect class pattern
class RainbowEffect : public BaseEffect {
public:
void update(uint32_t cycle) override {
// Effect logic here
for (int i = 0; i < numLeds; i++) {
leds[i] = CHSV((cycle + i \* 10) % 255, 255, 255);
}
FastLED.show();
}
};

// Effect registration
void registerEffects() {
effectRegistry.add("rainbow", new RainbowEffect());
effectRegistry.add("pulse", new PulseEffect());
}

// GPIO validation for LEDs
if (!isValidGPIO(LED_PIN) || !isSafeGPIO(LED_PIN)) {
logger.log("Invalid LED GPIO: " + String(LED_PIN));
return false;
}

#endif // ENABLE_LEDS
</patterns>

<common_tasks>
Adding new effect:

1. Create effect class with update() method
2. Use cycle parameter for timing consistency
3. Register in effect registry
4. Test memory usage on ESP32-C3
5. Add to web interface selection
   </common_tasks>

<fatal_implications>

- Missing #ifdef guards = Compile errors when LEDs disabled
- Skip GPIO validation = Hardware damage
- Poor cycle timing = Jerky animations
- Memory leaks = System crashes
  </fatal_implications>
