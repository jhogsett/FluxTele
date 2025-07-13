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
        _next_event_time = time + TELCO_TONE_A_DURATION;
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
            _next_event_time = time + TELCO_TONE_B_DURATION;
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
            _next_event_time = time + TELCO_TONE_A_DURATION;
            break;
    }
}

unsigned long AsyncTelco::get_random_silence_duration()
{
    // Generate random silence duration between min and max
    return TELCO_SILENCE_MIN + random(TELCO_SILENCE_MAX - TELCO_SILENCE_MIN);
}
