#ifndef __ASYNC_TELCO_H__
#define __ASYNC_TELCO_H__

#include <Arduino.h>

// Ring/Telco timing constants (in milliseconds)
// SimRing: North American telephone ring cadence (2 seconds on, 4 seconds off)
// SimPager: Authentic two-tone sequential timing (Genave/Motorola Quick Call)
#define TELCO_TONE_A_DURATION 2000    // Ring ON: 2.0 seconds (North American standard)
#define TELCO_TONE_B_DURATION 0       // Ring: Skip TONE B entirely (set to 0)
#define TELCO_INTER_TONE_GAP 0        // No gap between tones in authentic systems
#define TELCO_SILENCE_MIN 4000        // Ring OFF: 4.0 seconds (North American standard)
#define TELCO_SILENCE_MAX 4000        // Ring OFF: Fixed 4.0 seconds (no randomization for rings)

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

class AsyncTelco
{
public:
    AsyncTelco();

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
};

#endif
