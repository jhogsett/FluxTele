# FluxTele Usability Tweaks

This document tracks usability improvements made to enhance the user experience during operation. Each entry includes the problem, solution, and expected impact for future testing validation.

## Tweak #1: Station Manager Tuning Sensitivity

**Date**: July 19, 2025  
**Problem**: Station Manager was relocating stations too aggressively during tuning, causing frequent station movement that disrupted the listening experience.

**Root Cause**: The `PIPELINE_REALLOC_THRESHOLD` was designed for 10 Hz tuning steps, but FluxTele uses 100 Hz tuning steps. This meant stations were relocating every 30 tuning steps (3000 Hz ÷ 100 Hz = 30) instead of the intended behavior.

**Solution**: Increased `PIPELINE_REALLOC_THRESHOLD` from 3000 Hz to 6000 Hz in `include/station_manager.h`:
```cpp
#define PIPELINE_REALLOC_THRESHOLD 6000  // Reallocate when VFO moves 6 kHz (60 steps at 100Hz tuning)
```

**Expected Impact**: 
- Stations now relocate every 60 tuning steps instead of 30
- More stable tuning experience with less station movement
- Maintains dynamic station management while reducing aggressiveness
- Should feel more like a traditional radio receiver

**Testing Notes**: 
- Validate that tuning feels smoother and less disruptive
- Ensure stations still relocate appropriately when tuning far distances
- Check that the 10-station mixed configuration remains stable
- Verify no regression in station management functionality

---

## Tweak #2: Station Frequency Alignment to Tuning Steps

**Date**: July 19, 2025  
**Problem**: When StationManager moved stations to new frequencies, they could be placed at frequencies that don't align with the VFO tuning step size (100 Hz). This created a frustrating user experience where stations could be heard but couldn't be precisely tuned to using the knob.

**Root Cause**: The station reallocation logic in `reallocateStations()` and `activateStation()` calculated frequencies without considering the VFO tuning step boundaries. Additionally, the "operator frustration" self-relocation logic in `apply_operator_frustration_drift()` also created misaligned frequencies. A station could be placed at, for example, 555,123,450 Hz, but the user can only tune in 100 Hz steps, making it impossible to land precisely on that frequency.

**Solution**: 
1. Added `VFO_TUNING_STEP_SIZE` macro (100 Hz) to `include/station_manager.h`
2. Modified `reallocateStations()` and `activateStation()` to align all station frequencies to tuning step boundaries
3. Fixed `apply_operator_frustration_drift()` in both SimDTMF and SimTelco to align self-relocated frequencies:
```cpp
#define VFO_TUNING_STEP_SIZE 100         // VFO tuning step size in Hz - stations must align to these increments

// In reallocateStations() and activateStation():
uint32_t aligned_freq = (freq / VFO_TUNING_STEP_SIZE) * VFO_TUNING_STEP_SIZE;

// In apply_operator_frustration_drift():
const float VFO_STEP = 100.0f;  // Match VFO_TUNING_STEP_SIZE from StationManager
_fixed_freq = ((long)(new_freq / VFO_STEP)) * VFO_STEP;
```

**Expected Impact**:
- All relocated stations will be precisely tunable with the knob
- Eliminates frustrating "almost in tune" stations
- Maintains dynamic station management while ensuring usability
- Macro allows easy adjustment if tuning step size changes

**Testing Notes**:
- Verify that all stations can be precisely tuned to after StationManager moves them
- Check that stations remain tunable after "operator frustration" QSY events
- Ensure frequency alignment doesn't interfere with station spacing
- Ensure tuning experience feels more precise and satisfying
- Test with both tuning directions (up and down)
- Validate that both StationManager-moved and self-relocated stations align properly

---

## Tweak #3: Per-TelcoType Station Persistence Settings  

**Date**: July 19, 2025  
**Problem**: All SimTelco stations used the same hardcoded drift timing (`DRIFT_COUNT = 4`), creating uniform behavior across different telephony signal types. This made all stations feel the same rather than reflecting realistic operator behavior differences between signal types.

**Root Cause**: The single `DRIFT_COUNT` constant was used for all TelcoType stations with the formula `DRIFT_COUNT + random(DRIFT_COUNT)` (4-8 cycles). Different telephony signals should have different persistence characteristics - for example, busy signals might change more frequently than dial tones in realistic usage.

**Solution**: 
1. Replaced single `DRIFT_COUNT` with per-TelcoType arrays in `src/sim_telco.cpp`
2. Split into separate minimum and additional range settings for better control
3. Added helper function to calculate drift cycles based on TelcoType:
```cpp
// Per-TelcoType drift settings for realistic operator behavior
const int DRIFT_MIN_CYCLES[4] = {
    4,  // TELCO_RINGBACK - ringback signals are moderately persistent
    4,  // TELCO_BUSY - busy signals are moderately persistent  
    4,  // TELCO_REORDER - reorder signals are moderately persistent
    4   // TELCO_DIALTONE - dial tones are moderately persistent
};

const int DRIFT_ADDITIONAL_CYCLES[4] = {
    4,  // TELCO_RINGBACK - range: 4-8 cycles (same as before)
    4,  // TELCO_BUSY - range: 4-8 cycles (same as before)
    4,  // TELCO_REORDER - range: 4-8 cycles (same as before)  
    4   // TELCO_DIALTONE - range: 4-8 cycles (same as before)
};

int calculateDriftCycles(TelcoType type) {
    int type_index = (int)type;
    if (type_index >= 0 && type_index < 4) {
        return DRIFT_MIN_CYCLES[type_index] + random(DRIFT_ADDITIONAL_CYCLES[type_index]);
    }
    return 4 + random(4);  // Fallback
}
```

**Expected Impact**:
- Foundation for realistic per-signal persistence behavior
- Currently maintains same 4-8 cycle range for all types (no behavior change)
- Enables future fine-tuning of individual signal characteristics
- More maintainable and configurable drift logic

**Testing Notes**:
- Verify that all TelcoType stations still drift at same rate as before
- Test that different signal types can be assigned different values
- Ensure no regression in existing station behavior
- Validate array bounds checking works correctly
- Future: Adjust individual values per signal type for optimal listening experience

---

## Tweak #4: Eliminate Station Selection Bias in Dynamic Pipelining

**Date**: July 19, 2025  
**Problem**: When StationManager selected stations to relocate during dynamic pipelining, it showed selection bias toward stations with lower array indices (earlier stations in the configuration). When multiple stations were equally good candidates for relocation, the algorithm would always choose the one listed first.

**Root Cause**: The candidate selection process used a deterministic approach:
1. Find stations that can be moved → candidates array
2. Sort by distance (furthest first) → maintains array order for stations at same distance  
3. Move sequentially from candidates[0] → systematic bias toward earlier stations

This meant station 2 would always be moved before station 7 if they were at similar distances, creating predictable and potentially unnatural behavior patterns.

**Solution**: 
Added randomization to candidate selection while preserving distance-based priority in `src/station_manager.cpp`:
```cpp
// After initial distance sorting:
// 1. Shuffle entire candidates array to randomize selection
for (int i = candidate_count - 1; i > 0; --i) {
    int j = random(i + 1);  // Random index from 0 to i
    if (i != j) {
        StationDistance temp = candidates[i];
        candidates[i] = candidates[j];  
        candidates[j] = temp;
    }
}

// 2. Re-sort by distance to maintain furthest-first priority
// This preserves randomization within each distance group
```

**Expected Impact**:
- Eliminates systematic bias toward earlier stations in configuration
- Maintains proper distance-based prioritization (furthest stations moved first)
- Creates more natural and unpredictable station behavior
- All stations have equal probability of being selected when at similar distances
- Improves perceived randomness and realism of station management

**Testing Notes**:
- Observe station movement patterns over extended tuning sessions
- Verify that different stations get moved when multiple candidates are available
- Ensure furthest stations are still prioritized correctly
- Check that no single station dominates the movement pattern
- Validate that station behavior feels more natural and less predictable

---

## Future Tweak Ideas

**Potential Areas for Improvement** (to be tested and documented as implemented):

- **Display Responsiveness**: Evaluate scrolling speed and display timing constants
- **Signal Meter Sensitivity**: Assess charge pulse timing and decay rates
- **Encoder Sensitivity**: Review encoder detent settings and response curves  
- **Station Frequency Spacing**: Optimize `PIPELINE_STATION_SPACING` for telephony content
- **Audio Transition Smoothness**: Evaluate fade-in/fade-out during station changes
- **BFO Default Values**: Review default BFO offset for telephony applications
- **Station Activation Timing**: Assess delays in station begin()/end() cycles

---

## Testing Protocol

When testing usability tweaks:

1. **Baseline Recording**: Document current behavior before changes
2. **Controlled Testing**: Test with consistent frequency ranges and patterns
3. **User Experience Focus**: Evaluate from operator perspective, not just technical correctness
4. **Regression Testing**: Ensure changes don't break core functionality
5. **Documentation**: Record both positive and negative impacts
6. **Iteration**: Be prepared to refine values based on real-world usage

---

## Configuration Context

Current test configuration: `CONFIG_ALLTELCO`
- 10 stations total (8 SimTelco + 2 SimDTMF)
- 100 Hz tuning steps
- 4x AD9833 wave generators
- Mixed telephony content (ringback, dialtone, busy, DTMF)
