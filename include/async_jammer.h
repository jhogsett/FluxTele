#ifndef __ASYNC_JAMMER_H__
#define __ASYNC_JAMMER_H__

// Jammer state machine timing parameters (from PR documentation)
#define JAMMER_STEP_INTERVAL 50       // 50ms between updates
#define JAMMER_MAX_DRIFT 2000.0       // ±2kHz drift range  
#define JAMMER_STEP_SIZE 25.0         // 25Hz brownian steps
#define JAMMER_MUTE_PROBABILITY 15    // 15% chance to mute

// Jammer step return values (similar to async morse/pager patterns)
#define STEP_JAMMER_TURN_ON     1     // Turn transmitter on
#define STEP_JAMMER_TURN_OFF    2     // Turn transmitter off  
#define STEP_JAMMER_LEAVE_ON    3     // Continue transmitting
#define STEP_JAMMER_LEAVE_OFF   4     // Continue silence
#define STEP_JAMMER_CHANGE_FREQ 5     // Continue transmitting but change frequency

// Jammer transmission states
#define JAMMER_STATE_TRANSMITTING 0   // Currently jamming
#define JAMMER_STATE_MUTED       1    // Temporarily silent

class AsyncJammer
{
public:
    AsyncJammer();
    
    void start_jammer_transmission(bool repeat);
    int step_jammer(unsigned long time);
    int get_current_state() const { return _current_state; }
    float get_frequency_offset() const { return _frequency_offset; }
    
private:
    void apply_brownian_drift();
    void apply_boundary_enforcement();
    bool should_mute();
    unsigned long get_random_mute_duration();
    
    bool _active;                     // True when jammer is active
    bool _repeat;                     // True to repeat transmissions (jammers always repeat)
    bool _transmitting;               // True during transmission
    int _current_state;               // Current jammer state (TRANSMITTING, MUTED)
    unsigned long _next_event_time;   // When next state change should occur
    bool _initialized;                // True after first start_jammer_transmission call
    
    // Brownian motion drift state
    float _frequency_offset;          // Current frequency offset from base (-2kHz to +2kHz)
    float _velocity;                  // Current velocity for brownian motion
    unsigned long _last_drift_time;   // Last time drift was applied
};

#endif
