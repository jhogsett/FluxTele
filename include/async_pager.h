#ifndef __ASYNC_PAGER_H__
#define __ASYNC_PAGER_H__

#include <Arduino.h>

// Pager timing constants (in milliseconds) - authentic two-tone sequential timing
// Based on industry standards from Genave/Motorola Quick Call specifications
#define PAGER_TONE_A_DURATION 1000    // First tone: 1.0 seconds (standard duration)
#define PAGER_TONE_B_DURATION 3000    // Second tone: 3.0 seconds (standard duration)
#define PAGER_INTER_TONE_GAP 0        // No gap between tones in authentic systems
#define PAGER_SILENCE_MIN 3000        // Minimum 3 seconds silence
#define PAGER_SILENCE_MAX 6000        // Maximum 6 seconds silence (reduced for more frequent paging)

// Pager step return values
#define STEP_PAGER_TURN_ON   1        // Start transmitting (tone A or B)
#define STEP_PAGER_TURN_OFF  2        // Stop transmitting (enter silence)
#define STEP_PAGER_LEAVE_ON  3        // Continue transmitting (no change)
#define STEP_PAGER_LEAVE_OFF 4        // Continue silence
#define STEP_PAGER_CHANGE_FREQ 5      // Continue transmitting but change frequency

// Pager transmission states
#define PAGER_STATE_TONE_A   0        // Transmitting first tone (1 second)
#define PAGER_STATE_TONE_B   1        // Transmitting second tone (3 seconds)
#define PAGER_STATE_SILENCE  2        // Silent period between transmissions

class AsyncPager
{
public:
    AsyncPager();

    void start_pager_transmission(bool repeat);
    int step_pager(unsigned long time);
    int get_current_state() { return _current_state; }
    
private:
    void start_next_phase(unsigned long time);
    unsigned long get_random_silence_duration();

    bool _active;                     // True when pager is active
    bool _repeat;                     // True to repeat transmissions
    bool _transmitting;               // True during tone transmission
    int _current_state;               // Current pager state (TONE_A, TONE_B, SILENCE)
    unsigned long _next_event_time;   // When next state change should occur
    bool _initialized;                // True after first start_pager_transmission call
};

#endif
