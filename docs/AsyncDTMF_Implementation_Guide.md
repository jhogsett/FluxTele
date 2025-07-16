# AsyncDTMF and SimDTMF Implementation Guide

## Overview
This document captures the critical details learned while implementing AsyncDTMF and SimDTMF, following the proven SimTelco/AsyncTelco pattern. These lessons will be essential for implementing future telephony station types.

## Critical Architecture Patterns

### 1. Async Class Separation Pattern
**Key Insight**: Separate timing logic from wave generator management

```cpp
// AsyncDTMF handles ONLY timing and state transitions
class AsyncDTMF {
    // State machine for DTMF sequence timing
    // Returns action codes (STEP_DTMF_*) to SimDTMF
    int step_dtmf(unsigned long time);
};

// SimDTMF handles ONLY wave generator management
class SimDTMF : public SimDualTone {
    AsyncDTMF _dtmf;  // Composition, not inheritance
    // Responds to AsyncDTMF action codes
    bool step(unsigned long time) override;
};
```

**Why This Matters**: Clean separation makes debugging easier and follows the proven SimTelco pattern exactly.

### 2. Action Code Pattern
**Critical Detail**: Async classes return action codes, not direct control

```cpp
// AsyncDTMF returns these action codes:
#define STEP_DTMF_TURN_ON      1     // Start transmitting DTMF tone
#define STEP_DTMF_LEAVE_ON     2     // Continue transmitting same tone  
#define STEP_DTMF_TURN_OFF     3     // Stop transmitting (enter silence)
#define STEP_DTMF_LEAVE_OFF    4     // Continue silence
#define STEP_DTMF_CHANGE_FREQ  5     // Change to next digit frequency
#define STEP_DTMF_CYCLE_END    6     // End of sequence - release generators
```

**Implementation Pattern**:
```cpp
bool SimDTMF::step(unsigned long time) {
    int dtmf_state = _dtmf.step_dtmf(time);
    
    switch(dtmf_state) {
        case STEP_DTMF_TURN_ON:
            // Set frequencies, activate, send pulse
            break;
        case STEP_DTMF_CYCLE_END:
            // CRITICAL: Use end() not _active = false
            end();  // Properly releases realizers
            break;
    }
}
```

## Resource Management - The Most Critical Part

### 3. Proper Wave Generator Release
**CRITICAL MISTAKE**: Using `_active = false` and `realize()` 
**CORRECT SOLUTION**: Using `end()` method

```cpp
// WRONG - Doesn't actually free the realizers
case STEP_DTMF_CYCLE_END:
    _active = false;
    realize();  // Wave generators still allocated!
    break;

// CORRECT - Actually frees the realizers
case STEP_DTMF_CYCLE_END:
    end();  // Calls SimDualTone::end() -> Realization::end()
    break;
```

**Why This Matters**: 
- `_active = false` only deactivates generators, doesn't free them
- Signal meter still shows activity because generators are still allocated
- Next cycle can't acquire generators because they're "taken" but inactive
- `end()` actually calls `Realization::end()` which frees realizers back to pool

### 4. Wait Delay Restart Pattern
**Key Pattern**: Use wait delay mechanism for sequence restarts

```cpp
class SimDTMF {
    bool _in_wait_delay;            // True when waiting between cycles
    unsigned long _next_cycle_time; // Time to attempt restart
};

// In step() method:
if(_in_wait_delay && time >= _next_cycle_time) {
    if(begin(time)) {  // Try to re-acquire generators
        _in_wait_delay = false;
    } else {
        // Retry later with randomization
        _next_cycle_time = time + 500 + random(1000);
    }
}
```

**Why This Works**: Matches SimTelco's proven dynamic pipelining pattern exactly.

## Frequency Management

### 5. Dual Frequency Override Pattern
**Critical Detail**: Override frequency offset methods, not direct assignment

```cpp
class SimDTMF : public SimDualTone {
    float _current_row_freq;    // Current DTMF row frequency
    float _current_col_freq;    // Current DTMF column frequency
    
    // Override these methods from SimDualTone
    float getFrequencyOffsetA() const override {
        return _current_row_freq;  // Row frequency (low tone)
    }
    
    float getFrequencyOffsetC() const override {
        return _current_col_freq;  // Column frequency (high tone)
    }
};
```

**Why This Works**: SimDualTone's `force_frequency_update()` calls these methods automatically.

### 6. Frequency Setting Pattern
```cpp
void set_digit_frequencies(char digit) {
    int digit_index = char_to_digit_index(digit);
    int row_index = DIGIT_TO_ROW[digit_index];
    int col_index = DIGIT_TO_COL[digit_index];
    
    _current_row_freq = ROW_FREQUENCIES[row_index];
    _current_col_freq = COL_FREQUENCIES[col_index];
    // No direct wavegen calls - let SimDualTone handle it
}
```

## State Machine Design

### 7. Action Code Return Strategy
**Pattern**: Return action codes immediately on state transitions

```cpp
int AsyncDTMF::step_dtmf(unsigned long time) {
    if (time < _next_event_time) {
        // Not time for transition yet
        return _transmitting ? STEP_DTMF_LEAVE_ON : STEP_DTMF_LEAVE_OFF;
    }
    
    // Time for transition - return action immediately
    switch(_dtmf_state) {
        case DTMF_IDLE:
            _dtmf_state = DTMF_PLAYING_TONE;
            _transmitting = true;
            _next_event_time = time + DTMF_TONE_DURATION;
            return STEP_DTMF_TURN_ON;  // Immediate action
    }
}
```

### 8. Silent Period Management
**Key Insight**: Different silence types need different handling

```cpp
case STEP_DTMF_TURN_OFF:
    // Inter-digit silence - keep generators, set silent frequencies
    _current_row_freq = 0.0f;
    _current_col_freq = 0.0f;
    force_frequency_update();
    // Note: Keep _active = true to maintain allocation
    break;
    
case STEP_DTMF_CYCLE_END:
    // Inter-sequence pause - release generators completely
    end();  // Actually free the realizers
    _in_wait_delay = true;
    break;
```

## Integration with SimDualTone

### 9. Constructor Pattern
```cpp
SimDTMF::SimDTMF(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, 
                 float fixed_freq, const char* sequence)
    : SimDualTone(wave_gen_pool, fixed_freq), 
      _signal_meter(signal_meter), 
      _digit_sequence(sequence) 
{
    // Initialize AsyncDTMF state
    _current_row_freq = 0.0f;
    _current_col_freq = 0.0f;
    _in_wait_delay = false;
    _next_cycle_time = 0;
}
```

### 10. Begin Method Pattern
```cpp
bool SimDTMF::begin(unsigned long time) {
    // Standard SimDualTone initialization
    if(!common_begin(time, _fixed_freq)) {
        return false;
    }
    
    // Initialize generators to silent
    // Set enabled, force update, realize (CRITICAL!)
    _enabled = true;
    force_frequency_update();
    realize();  // CRITICAL: Set active state for audio output!
    
    // Start AsyncDTMF sequence
    _dtmf.start_dtmf_transmission(_digit_sequence, true);
    _in_wait_delay = false;
    
    return true;
}
```

## Debugging Insights

### 11. Signal Meter as Diagnostic Tool
**Key Observation**: Signal meter activity without audio means generators allocated but inactive
- If signal meter shows activity but no audio → generators allocated but `_active = false`
- If no signal meter activity → generators not allocated (proper `end()` called)
- This was the critical clue that led to discovering the `end()` vs `_active = false` issue

### 12. Visual Indicators
- Station lock LED + signal meter activity = generators allocated
- Audio output = generators allocated AND active
- Missing realize() call = allocated but never activated

## Template for Future Station Types

### 13. General Pattern for New Async Station Types
```cpp
// 1. Create AsyncXXX class for timing
class AsyncBusy {
    int step_busy(unsigned long time);  // Returns STEP_BUSY_* codes
};

// 2. Create SimXXX class inheriting SimDualTone  
class SimBusy : public SimDualTone {
    AsyncBusy _busy;
    bool _in_wait_delay;
    unsigned long _next_cycle_time;
    
public:
    bool step(unsigned long time) override {
        int state = _busy.step_busy(time);
        switch(state) {
            case STEP_BUSY_CYCLE_END:
                end();  // CRITICAL: Use end() not _active = false
                _in_wait_delay = true;
                break;
        }
        
        // Wait delay restart logic (copy from SimDTMF)
        if(_in_wait_delay && time >= _next_cycle_time) {
            if(begin(time)) {
                _in_wait_delay = false;
            } else {
                _next_cycle_time = time + 500 + random(1000);
            }
        }
    }
};
```

## Common Pitfalls to Avoid

1. **Using `_active = false` instead of `end()`** - Generators stay allocated
2. **Missing `realize()` call in `begin()`** - Generators never activate  
3. **Not implementing wait delay restart logic** - Station dies after first cycle
4. **Direct wavegen frequency assignment** - Bypasses SimDualTone frequency management
5. **Forgetting to call parent `common_begin()`** - Wave generator acquisition fails
6. **Missing `force_frequency_update()` after frequency changes** - Old frequencies persist

## Success Verification Checklist

- [ ] First sequence/cycle plays completely ✅
- [ ] Silent pause occurs (visual indicators show generator release) ✅  
- [ ] Second sequence/cycle starts automatically ✅
- [ ] Continuous operation without manual intervention ✅
- [ ] Signal meter shows activity only during transmission ✅
- [ ] Station lock LED correlates with audio output ✅

This pattern should work for any telephony station type (busy signals, dial tones, ring back, etc.) by following the AsyncDTMF/SimDTMF architecture exactly.
