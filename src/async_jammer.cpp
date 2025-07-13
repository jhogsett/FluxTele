#include "async_jammer.h"
#include <Arduino.h>

AsyncJammer::AsyncJammer()
{
    // Initialize all state variables to safe defaults
    _active = false;
    _repeat = false;
    _transmitting = false;
    _current_state = JAMMER_STATE_MUTED;
    _next_event_time = 0;
    _initialized = false;
    
    // Initialize brownian motion state
    _frequency_offset = 0.0;
    _velocity = 0.0;
    _last_drift_time = 0;
}

void AsyncJammer::start_jammer_transmission(bool repeat)
{
    _repeat = repeat;
    _active = true;
    _initialized = true;
    
    // Jammers start transmitting immediately
    _current_state = JAMMER_STATE_TRANSMITTING;
    _transmitting = true;
    
    // Initialize to 0 like other async classes - timing will be set on first step_jammer call
    _next_event_time = 0;
    _last_drift_time = 0;
}

int AsyncJammer::step_jammer(unsigned long time)
{
    if (!_active || !_initialized) {
        return STEP_JAMMER_LEAVE_OFF;
    }
    
    // Apply brownian drift at regular intervals
    if (time - _last_drift_time >= JAMMER_STEP_INTERVAL) {
        apply_brownian_drift();
        _last_drift_time = time;
    }
    
    // If this is the first call (next_event_time is 0), set up initial timing
    if (_next_event_time == 0) {
        _next_event_time = time + JAMMER_STEP_INTERVAL;
        return STEP_JAMMER_TURN_ON;  // Start transmitting
    }
    
    // Check if it's time for a state change
    if (time < _next_event_time) {
        // Not time to change state yet, but frequency may have drifted
        if (_transmitting && _current_state == JAMMER_STATE_TRANSMITTING) {
            // Check if frequency drifted significantly enough to report change
            static float last_reported_offset = 0.0;
            if (abs(_frequency_offset - last_reported_offset) > JAMMER_STEP_SIZE) {
                last_reported_offset = _frequency_offset;
                return STEP_JAMMER_CHANGE_FREQ;
            }
            return STEP_JAMMER_LEAVE_ON;
        } else {
            return STEP_JAMMER_LEAVE_OFF;
        }
    }
      // Time to potentially change state
    bool was_transmitting = _transmitting;
    
    // Check for random muting
    if (_transmitting && should_mute()) {
        // Start mute period
        _current_state = JAMMER_STATE_MUTED;
        _transmitting = false;
        _next_event_time = time + get_random_mute_duration();
    } else if (!_transmitting) {
        // End mute period, return to transmitting
        _current_state = JAMMER_STATE_TRANSMITTING;
        _transmitting = true;
        _next_event_time = time + JAMMER_STEP_INTERVAL;
    } else {
        // Continue transmitting, just update timing
        _next_event_time = time + JAMMER_STEP_INTERVAL;
    }
    
    // Return appropriate step based on transition
    if (_transmitting && !was_transmitting) {
        return STEP_JAMMER_TURN_ON;     // OFF → ON
    } else if (!_transmitting && was_transmitting) {
        return STEP_JAMMER_TURN_OFF;    // ON → OFF
    } else if (_transmitting) {
        return STEP_JAMMER_LEAVE_ON;    // ON → ON (may include frequency drift)
    } else {
        return STEP_JAMMER_LEAVE_OFF;   // OFF → OFF
    }
}

void AsyncJammer::apply_brownian_drift()
{
    // Apply velocity-based brownian motion for realistic frequency wandering
    
    // Add random velocity change (brownian motion)
    float velocity_change = (random(1000) / 1000.0 - 0.5) * JAMMER_STEP_SIZE;
    
    _velocity += velocity_change;
    
    // Apply damping to prevent excessive velocity buildup
    _velocity *= 0.95;
    
    // Update frequency offset based on velocity
    _frequency_offset += _velocity;
    
    // Apply boundary enforcement
    apply_boundary_enforcement();
}

void AsyncJammer::apply_boundary_enforcement()
{
    // Soft boundary enforcement with velocity damping and bouncing
    if (_frequency_offset > JAMMER_MAX_DRIFT) {
        _frequency_offset = JAMMER_MAX_DRIFT;
        _velocity = -abs(_velocity) * 0.5;  // Bounce back with reduced velocity
    } else if (_frequency_offset < -JAMMER_MAX_DRIFT) {
        _frequency_offset = -JAMMER_MAX_DRIFT;
        _velocity = abs(_velocity) * 0.5;   // Bounce back with reduced velocity
    }
}

bool AsyncJammer::should_mute()
{
    // 15% probability of temporary silence for realistic interference
    return random(100) < JAMMER_MUTE_PROBABILITY;
}

unsigned long AsyncJammer::get_random_mute_duration()
{
    // Random mute duration between 20-200ms for realistic jamming behavior
    return 20 + random(181);     // 20-200ms
}
