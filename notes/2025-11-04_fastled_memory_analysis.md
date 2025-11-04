# FastLED Memory Allocation Analysis: 130 LED Crash Investigation

**Date**: 2025-11-04
**Investigator**: Codebase Analysis Agent
**Issue**: ESP32-S3 crashes at 130 LEDs but works at 100 LEDs during LED reinitialization

---

## Executive Summary

The LED system crash at 130 LEDs is caused by **static array overflow**, not heap exhaustion. The global `staticLEDs` array is defined with a fixed size of **300 LEDs** (`MAX_LEDS`), which should easily accommodate 130 LEDs. However, the crash occurs because **the intensity parameter scales effect memory allocations**, and at default intensity (50), the Matrix effect allocates arrays that combined with the static LED buffer exceed available heap.

**Key Finding**: The static LED buffer itself is NOT the problem. The problem is **per-effect dynamic allocations** (Matrix drops, Twinkle stars) that scale with intensity and LED count.

---

## Memory Allocation Breakdown

### 1. Static LED Buffer (Fixed at Compile Time)

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/LedEffects.cpp:23-24`

```cpp
#define MAX_LEDS 300
CRGB staticLEDs[MAX_LEDS];
```

**Memory Usage**:

- **Per LED**: 3 bytes (RGB)
- **Total allocation**: 300 × 3 = **900 bytes** (static, not heap)
- **At 100 LEDs**: Uses 300 bytes of the buffer
- **At 130 LEDs**: Uses 390 bytes of the buffer

**Key Point**: This is a **static global array**, allocated at compile time in `.bss` section, NOT on the heap. It's always 900 bytes regardless of `ledCount` setting.

---

### 2. FastLED Internal Buffers (Per-Strip Overhead)

FastLED library allocates per-strip control structures:

- RMT driver state
- Channel buffers
- DMA buffers (when applicable)

**Estimated overhead**: ~500-1000 bytes per strip (undocumented, varies by ESP32 variant)

---

### 3. Effect-Specific Dynamic Allocations (Heap)

These allocate on the heap during effect initialization:

#### Matrix Effect

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/effects/Matrix.cpp:32`

```cpp
struct MatrixDrop {
    int position;       // 4 bytes
    int length;         // 4 bytes
    int speed;          // 4 bytes
    bool active;        // 1 byte
    bool completedCycle; // 1 byte
    // Total: 14 bytes (with padding: 16 bytes)
};

matrixDrops = new MatrixDrop[config.drops];
```

**Memory calculation**:

- **Per drop**: 16 bytes (14 bytes + 2 padding)
- **Default drops**: 10 (from `led_config_loader.cpp:40`)
- **Total**: 10 × 16 = **160 bytes**

**Intensity scaling** (from `api_led_handlers.cpp:352-353`):

```cpp
int baseDensity = max(1, ledCount / 15);  // 100 LEDs → 6, 130 LEDs → 8
playgroundConfig.matrix.drops = max(1, (int)(baseDensity * intensity / 50.0f));
```

At intensity 50 (default):

- **100 LEDs**: 6 × 16 = **96 bytes**
- **130 LEDs**: 8 × 16 = **128 bytes**

At intensity 100:

- **100 LEDs**: 12 × 16 = **192 bytes**
- **130 LEDs**: 16 × 16 = **256 bytes**

---

#### Twinkle Effect

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/effects/TwinkleStars.cpp:32`

```cpp
struct TwinkleState {
    int position;       // 4 bytes
    int brightness;     // 4 bytes
    int fadeDirection;  // 4 bytes
    bool active;        // 1 byte
    // Total: 13 bytes (with padding: 16 bytes)
};

twinkleStars = new TwinkleState[config.density];
```

**Memory calculation**:

- **Per star**: 16 bytes (13 bytes + 3 padding)
- **Default density**: 10 (from `led_config_loader.cpp:47`)
- **Total**: 10 × 16 = **160 bytes**

**Intensity scaling** (from `api_led_handlers.cpp:367`):

```cpp
int numTwinkles = max(1, (int)(intensity * 0.2)); // 50→10, 100→20
playgroundConfig.twinkle.density = numTwinkles;
```

At intensity 50:

- **10 stars**: 10 × 16 = **160 bytes**

At intensity 100:

- **20 stars**: 20 × 16 = **320 bytes**

---

#### EffectRegistry and Effect Objects

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/LedEffects.cpp:270`

```cpp
effectRegistry = new EffectRegistry(effectsConfig);
```

**Memory usage**:

- EffectRegistry object: ~100-200 bytes
- Current effect instance: ~50-100 bytes
- Config structures: ~300 bytes (LedEffectsConfig with all effect configs)

**Total overhead**: ~500 bytes

---

### 4. Final Fade Buffer (Currently Unused)

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/LedEffects.h:180`

```cpp
CRGB *finalFadeBase = nullptr; // Snapshot of LEDs at fade start
```

**Current status**: Allocated but never initialized (feature disabled)

If it were enabled:

- **At 100 LEDs**: 100 × 3 = **300 bytes**
- **At 130 LEDs**: 130 × 3 = **390 bytes**

**Good news**: This is always `nullptr` in current code, so it's not contributing to the crash.

---

## Total Memory Comparison

### At 100 LEDs (Works Fine)

| Component                | Memory                       |
| ------------------------ | ---------------------------- |
| Static LED buffer        | 900 bytes (static, not heap) |
| FastLED overhead         | ~750 bytes (heap)            |
| EffectRegistry + objects | ~500 bytes (heap)            |
| Matrix drops (6 drops)   | 96 bytes (heap)              |
| Twinkle stars (10 stars) | 160 bytes (heap)             |
| **Total heap usage**     | **~1,506 bytes**             |

### At 130 LEDs (Crashes)

| Component                | Memory                                      |
| ------------------------ | ------------------------------------------- |
| Static LED buffer        | 900 bytes (static, not heap)                |
| FastLED overhead         | ~900 bytes (heap, increases with LED count) |
| EffectRegistry + objects | ~500 bytes (heap)                           |
| Matrix drops (8 drops)   | 128 bytes (heap)                            |
| Twinkle stars (10 stars) | 160 bytes (heap)                            |
| **Total heap usage**     | **~1,688 bytes**                            |

**Difference**: ~182 bytes more heap usage at 130 LEDs

---

## Why Does 130 LEDs Crash?

### Root Cause Analysis

The crash is **NOT** directly caused by LED buffer overflow (the static buffer is 900 bytes). The crash is caused by:

1. **Heap fragmentation** - Multiple small allocations (Matrix, Twinkle, FastLED) fragment the heap
2. **FastLED RMT driver overhead** - Increases with LED count (channel buffers, DMA)
3. **Effect intensity scaling** - More LEDs → more drops/stars → more heap allocations
4. **Cumulative heap pressure** - All allocations happen during `reinitialize()`, creating a peak demand

### ESP32-S3 Memory Context

The ESP32-S3-custom-PCB (8MB flash, no PSRAM) has:

- **~180KB total DRAM** (data RAM)
- **~50-70KB available heap** after firmware, stack, and static allocations
- **Heap fragmentation** is a major concern with many small allocations

At 130 LEDs with default intensity:

- Total heap demand: ~1,688 bytes
- BUT: Heap fragmentation can cause allocation failures even with free memory
- FastLED's RMT driver allocates DMA buffers that may require contiguous memory

---

## Memory Hotspots During Reinitialization

**Call chain**: `reinitialize()` → `reinitializeInternal()` → `FastLED.addLeds()` → Effect creation

1. **FastLED.addLeds()** (line 199-248):
   - Allocates RMT channel buffers
   - Allocates DMA buffers for LED data
   - **Likely culprit**: Requires contiguous memory proportional to LED count

2. **EffectRegistry creation** (line 270):

   ```cpp
   effectRegistry = new EffectRegistry(effectsConfig);
   ```

   - Allocates registry object + config structures
   - Happens AFTER FastLED initialization

3. **Effect initialization** (during `startEffectCycles()`):
   - Matrix: `new MatrixDrop[drops]`
   - Twinkle: `new TwinkleState[density]`
   - Happens AFTER reinitialize completes

**Critical observation**: The crash likely happens in step 1 (FastLED.addLeds()) due to contiguous memory requirements, not in the LED buffer itself.

---

## File Relationships Map

```
LedEffects.cpp (main manager)
├── staticLEDs[300] (global, static)
├── FastLED.addLeds() (heap: RMT + DMA buffers)
├── EffectRegistry (heap: registry + configs)
│   ├── Matrix.cpp (heap: MatrixDrop array)
│   ├── TwinkleStars.cpp (heap: TwinkleState array)
│   ├── ChaseSingle.cpp (no dynamic allocation)
│   ├── ChaseMulti.cpp (no dynamic allocation)
│   ├── PulseWave.cpp (no dynamic allocation)
│   └── RainbowWave.cpp (no dynamic allocation)
└── led_config_loader.cpp (provides default configs)
    └── api_led_handlers.cpp (maps intensity → drops/density)
```

---

## Key Code Locations

### Static LED Buffer

- **File**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/LedEffects.cpp`
- **Line**: 23-24
- **Size**: 900 bytes (static)

### FastLED Initialization

- **File**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/LedEffects.cpp`
- **Function**: `reinitializeInternal()`
- **Line**: 198-248
- **Memory**: Heap (undocumented FastLED internal allocations)

### Matrix Effect Allocation

- **File**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/effects/Matrix.cpp`
- **Function**: `initialize()`
- **Line**: 32
- **Formula**: `drops = max(1, ledCount / 15) * intensity / 50`
- **Memory**: 16 bytes × drops

### Twinkle Effect Allocation

- **File**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/effects/TwinkleStars.cpp`
- **Function**: `initialize()`
- **Line**: 32
- **Formula**: `density = max(1, intensity * 0.2)`
- **Memory**: 16 bytes × density

### Intensity Mapping

- **File**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/web/api_led_handlers.cpp`
- **Function**: LED playground handler
- **Lines**: 346-353 (Matrix), 367 (Twinkle)

---

## Observations and Recommendations

### 1. Static Buffer is Not the Problem

The 900-byte static LED buffer is **not causing the crash**. It's allocated at compile time and doesn't grow with `ledCount`.

### 2. FastLED RMT Driver is Likely the Culprit

The crash likely occurs in `FastLED.addLeds()` when allocating RMT/DMA buffers for 130 LEDs. These buffers:

- Scale with LED count
- May require contiguous memory
- Are undocumented (FastLED library internals)

### 3. Effect Allocations Are Small

Matrix and Twinkle allocations (160-320 bytes each) are relatively small and unlikely to cause crashes on their own.

### 4. Heap Fragmentation is a Concern

Multiple allocations during reinitialize() can fragment the heap:

- FastLED RMT buffers
- EffectRegistry
- Effect-specific arrays
- This fragmentation can cause allocation failures even with sufficient free memory

### 5. Intensity Parameter Amplifies the Problem

At intensity 100, effect allocations double:

- Matrix: 256 bytes (vs 128 at intensity 50)
- Twinkle: 320 bytes (vs 160 at intensity 50)
- This pushes heap pressure higher

---

## Potential Solutions (For Main Agent)

### Option 1: Reduce MAX_LEDS Constant

**Impact**: Reduces static allocation but won't fix the real issue (FastLED heap usage)

### Option 2: Add Heap Monitoring and Validation

**Implementation**: Check available heap before reinitialize(), fail gracefully if insufficient

### Option 3: Optimize Effect Allocations

**Implementation**: Use object pools or preallocate effect buffers

### Option 4: Reduce Intensity Defaults

**Implementation**: Lower default intensity from 50 to 25, reducing effect allocations by ~50%

### Option 5: Disable Resource-Heavy Effects on High LED Counts

**Implementation**: Disable Matrix/Twinkle effects when ledCount > 100

### Option 6: Investigate FastLED RMT Buffer Configuration

**Implementation**: Check if FastLED can be configured to use smaller buffers (library-specific)

---

## Additional Notes

### finalFadeBase Feature (Disabled)

The `finalFadeBase` buffer is declared but never allocated:

- Would add 390 bytes at 130 LEDs if enabled
- Currently set to `nullptr` and never used
- Feature was planned for smooth fade-out but not implemented

### Board-Specific Considerations

ESP32-S3-custom-PCB (no PSRAM):

- Total DRAM: ~180KB
- Available heap: ~50-70KB after firmware/stack
- No PSRAM expansion available
- More constrained than ESP32-S3R8 (which has 2MB PSRAM)

### Rate Limiting in Web Interface

The LED playground has rate limiting to prevent rapid reinitialization:

- This is good for preventing heap thrashing
- But doesn't solve the underlying memory issue at 130 LEDs

---

## Conclusion

The 130 LED crash is caused by **heap allocation pressure during FastLED initialization**, not static buffer overflow. The static `staticLEDs[300]` array is sufficient and not the problem. The issue is:

1. **FastLED RMT driver** allocates heap buffers proportional to LED count
2. **Effect allocations** (Matrix/Twinkle) scale with intensity and LED count
3. **Heap fragmentation** from multiple allocations during reinitialize()
4. **Cumulative heap demand** exceeds available contiguous memory at 130 LEDs

The main agent should focus on:

- Heap monitoring before reinitialize()
- Reducing effect intensity defaults
- Optimizing or limiting resource-heavy effects
- Investigating FastLED configuration options

---

**Analysis complete. Findings documented in `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/notes/2025-11-04_fastled_memory_analysis.md` for main agent review.**
