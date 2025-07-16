#ifndef __ASYNC_DTMF2_H__
#define __ASYNC_DTMF2_H__

#include <Arduino.h>
#include "telco_types.h"

// Ring/Telco timing constants (in milliseconds) - now supports multiple patterns
// Ringback cadence (North American standard)
#define RINGBACK_TONE_A_DURATION 2000    // Ringback ON: 2.0 seconds 
#define RINGBACK_TONE_B_DURATION 0       // Ringback: Skip TONE B entirely (set to 0)
#define RINGBACK_SILENCE_MIN 4000        // Ringback OFF: 4.0 seconds
#define RINGBACK_SILENCE_MAX 4000        // Ringback OFF: Fixed 4.0 seconds (no randomization)

// Busy cadence (North American standard)
#define BUSY_TONE_A_DURATION 500     // Busy ON: 0.5 seconds
#define BUSY_TONE_B_DURATION 0       // Busy: Skip TONE B entirely (set to 0)  
#define BUSY_SILENCE_MIN 500         // Busy OFF: 0.5 seconds
#define BUSY_SILENCE_MAX 500         // Busy OFF: Fixed 0.5 seconds

// Reorder cadence (North American standard)
#define REORDER_TONE_A_DURATION 250  // Reorder ON: 0.25 seconds
#define REORDER_TONE_B_DURATION 0    // Reorder: Skip TONE B entirely (set to 0)
#define REORDER_SILENCE_MIN 250      // Reorder OFF: 0.25 seconds  
#define REORDER_SILENCE_MAX 250      // Reorder OFF: Fixed 0.25 seconds

// Dial tone cadence (North American standard)
#define DIALTONE_TONE_A_DURATION 15000  // Dial tone ON: 15.0 seconds
#define DIALTONE_TONE_B_DURATION 0      // Dial tone: Skip TONE B entirely (set to 0)
#define DIALTONE_SILENCE_MIN 2000       // Dial tone OFF: 2.0 seconds
#define DIALTONE_SILENCE_MAX 2000       // Dial tone OFF: Fixed 2.0 seconds

// Legacy constants for backward compatibility
#define TELCO_TONE_A_DURATION RINGBACK_TONE_A_DURATION    
#define TELCO_TONE_B_DURATION RINGBACK_TONE_B_DURATION       
#define TELCO_INTER_TONE_GAP 0        // No gap between tones in authentic systems
#define TELCO_SILENCE_MIN RINGBACK_SILENCE_MIN        
#define TELCO_SILENCE_MAX RINGBACK_SILENCE_MAX

// Legacy ring constants (deprecated - use RINGBACK)
#define RING_TONE_A_DURATION RINGBACK_TONE_A_DURATION
#define RING_TONE_B_DURATION RINGBACK_TONE_B_DURATION
#define RING_SILENCE_MIN RINGBACK_SILENCE_MIN
#define RING_SILENCE_MAX RINGBACK_SILENCE_MAX

// Telco step return values
#define STEP_TELCO_TURN_ON   1        // Start transmitting (tone A or B)
#define STEP_TELCO_TURN_OFF  2        // Stop transmitting (enter silence)
#define STEP_TELCO_LEAVE_ON  3        // Continue transmitting (no change)
#define STEP_TELCO_LEAVE_OFF 4        // Continue silence
#define STEP_TELCO_CHANGE_FREQ 5      // Continue transmitting but change frequency

// Pager transmission states
#define PAGER_STATE_TONE_A   0        // Transmitting first tone (1 second)
#define PAGER_STATE_TONE_B   1        // Transmitting second tone (3 seconds)
#define PAGER_STATE_SILENCE  2        // Silent period between transmissions

#define TELCO_STATE_TONE_A   0        // Transmitting tone A
#define TELCO_STATE_TONE_B   1        // Transmitting tone B  
#define TELCO_STATE_SILENCE  2        // Silent period between transmissions

class AsyncDTMF2
{
public:
    AsyncDTMF2();

    void configure_timing(TelcoType type);  // Configure timing based on telco type
    void start_telco_transmission(bool repeat);
    int step_telco(unsigned long time);
    int get_current_state() { return _current_state; }
    
private:
    void start_next_phase(unsigned long time);
    unsigned long get_random_silence_duration();

    bool _active;                     // True when telco is active
    bool _repeat;                     // True to repeat transmissions
    bool _transmitting;               // True during tone transmission
    int _current_state;               // Current telco state (TONE_A, TONE_B, SILENCE)
    unsigned long _next_event_time;   // When next state change should occur
    bool _initialized;                // True after first start_telco_transmission call
    
    // Configurable timing parameters (set by configure_timing)
    unsigned long _tone_a_duration;   // Duration of tone A (ms)
    unsigned long _tone_b_duration;   // Duration of tone B (ms) - 0 for single tone
    unsigned long _silence_min;       // Minimum silence duration (ms)
    unsigned long _silence_max;       // Maximum silence duration (ms)
};

#endif
