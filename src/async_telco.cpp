#include <Arduino.h>

#include "../include/async_telco.h"

AsyncTelco::AsyncTelco()
{    // Initialize all state variables to safe defaults
    _active = false;
    _repeat = false;
    _transmitting = false;
    _current_state = TELCO_STATE_SILENCE;
    _next_event_time = 0;
    _initialized = false;
    
    // Default to ring timing for backward compatibility
    _tone_a_duration = RING_TONE_A_DURATION;
    _tone_b_duration = RING_TONE_B_DURATION;
    _silence_min = RING_SILENCE_MIN;
    _silence_max = RING_SILENCE_MAX;
}

void AsyncTelco::configure_timing(TelcoType type)
{
    switch (type) {
        case TELCO_RINGBACK:
            _tone_a_duration = RINGBACK_TONE_A_DURATION;   // 2000ms (2s)
            _tone_b_duration = RINGBACK_TONE_B_DURATION;   // 0ms (single tone)
            _silence_min = RINGBACK_SILENCE_MIN;           // 4000ms (4s)
            _silence_max = RINGBACK_SILENCE_MAX;           // 4000ms (4s)
            break;
            
        case TELCO_BUSY:
            _tone_a_duration = BUSY_TONE_A_DURATION;   // 500ms (0.5s)
            _tone_b_duration = BUSY_TONE_B_DURATION;   // 0ms (single tone)
            _silence_min = BUSY_SILENCE_MIN;           // 500ms (0.5s)
            _silence_max = BUSY_SILENCE_MAX;           // 500ms (0.5s)
            break;
            
        case TELCO_REORDER:
            _tone_a_duration = REORDER_TONE_A_DURATION; // 250ms (0.25s)
            _tone_b_duration = REORDER_TONE_B_DURATION; // 0ms (single tone)
            _silence_min = REORDER_SILENCE_MIN;         // 250ms (0.25s)
            _silence_max = REORDER_SILENCE_MAX;         // 250ms (0.25s)
            break;
            
        case TELCO_DIALTONE:
            _tone_a_duration = DIALTONE_TONE_A_DURATION; // 15000ms (15s)
            _tone_b_duration = DIALTONE_TONE_B_DURATION; // 0ms (single tone)
            _silence_min = DIALTONE_SILENCE_MIN;         // 2000ms (2s)
            _silence_max = DIALTONE_SILENCE_MAX;         // 2000ms (2s)
            break;
            
        default:
            // Default to ringback timing
            _tone_a_duration = RINGBACK_TONE_A_DURATION;
            _tone_b_duration = RINGBACK_TONE_B_DURATION;
            _silence_min = RINGBACK_SILENCE_MIN;
            _silence_max = RINGBACK_SILENCE_MAX;
            break;
    }
}

void AsyncTelco::start_telco_transmission(bool repeat)
{
    _repeat = repeat;
    _active = true;
    _initialized = true;
    
    // Start with first tone immediately
    _current_state = TELCO_STATE_TONE_A;
    _transmitting = true;
    
    // Initialize to 0 like morse class - timing will be set on first step_telco call
    _next_event_time = 0;
}

int AsyncTelco::step_telco(unsigned long time)
{
    if (!_active || !_initialized) {
        return STEP_TELCO_LEAVE_OFF;
    }
      // If this is the first call (next_event_time is 0), set up initial timing
    if (_next_event_time == 0) {
        _next_event_time = time + _tone_a_duration;  // Use configurable timing
        return STEP_TELCO_TURN_ON;  // Start transmitting Tone A
    }
    
    // Check if it's time for a state change
    if (time < _next_event_time) {
        // Not time to change state yet
        return _transmitting ? STEP_TELCO_LEAVE_ON : STEP_TELCO_LEAVE_OFF;
    }    // Time to change state
    bool was_transmitting = _transmitting;
    int old_state = _current_state;
    start_next_phase(time);
    
    // Check if telco became inactive after state change (no-repeat case)
    if (!_active) {
        return STEP_TELCO_LEAVE_OFF;
    }
    
    // Return appropriate step based on transition
    if (_transmitting && !was_transmitting) {
        return STEP_TELCO_TURN_ON;     // OFF → ON
    } else if (!_transmitting && was_transmitting) {
        return STEP_TELCO_TURN_OFF;    // ON → OFF  
    } else if (_transmitting && was_transmitting && _current_state != old_state) {
        return STEP_TELCO_CHANGE_FREQ; // ON → ON with frequency change (Tone A → Tone B)
    } else if (_transmitting) {
        return STEP_TELCO_LEAVE_ON;    // ON → ON (no change)
    } else {
        return STEP_TELCO_LEAVE_OFF;   // OFF → OFF
    }
}

void AsyncTelco::start_next_phase(unsigned long time)
{
    switch (_current_state) {        case TELCO_STATE_TONE_A:
            // Tone A finished, start Tone B immediately (no gap)
            _current_state = TELCO_STATE_TONE_B;
            _transmitting = true;
            _next_event_time = time + _tone_b_duration;  // Use configurable timing
            break;            
        case TELCO_STATE_TONE_B:
            // Tone B finished, start silence period
            _current_state = TELCO_STATE_SILENCE;
            _transmitting = false;
            if (_repeat) {
                _next_event_time = time + get_random_silence_duration();
            } else {
                // No repeat: become inactive immediately, no future events
                _active = false;
                _next_event_time = 0; // No future events
            }
            break;        case TELCO_STATE_SILENCE:
            // Silence finished (only reachable if _repeat is true)
            // Start next transmission cycle with Tone A
            _current_state = TELCO_STATE_TONE_A;
            _transmitting = true;
            _next_event_time = time + _tone_a_duration;  // Use configurable timing
            break;
    }
}

unsigned long AsyncTelco::get_random_silence_duration()
{
    // Generate random silence duration between min and max (using configurable timing)
    return _silence_min + random(_silence_max - _silence_min);
}
