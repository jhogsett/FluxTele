#ifndef __ASYNC_DTMF_H__
#define __ASYNC_DTMF_H__

#include <Arduino.h>

// AsyncDTMF - DTMF sequence timing manager (similar to AsyncTelco)
// Handles the timing and state transitions for DTMF digit sequences

// DTMF timing constants - Human-like variability for realistic touch-tone dialing
#define DTMF_TONE_MIN_DURATION     100   // Minimum tone duration (humans hold buttons longer)
#define DTMF_TONE_MAX_DURATION     400   // Maximum tone duration (natural variation)
#define DTMF_SILENCE_MIN_DURATION  50   // Minimum silence between tones
#define DTMF_SILENCE_MAX_DURATION  100   // Maximum silence between tones
#define DTMF_DIGIT_GAP_MIN         100   // Fast dialing for repeated digits
#define DTMF_DIGIT_GAP_MAX         100   // Thinking pause for new digit groups
#define DTMF_SEQUENCE_GAP          3000  // 3 second gap between sequence repeats

// Return values for step_dtmf() method
#define STEP_DTMF_TURN_ON      1     // Start transmitting DTMF tone
#define STEP_DTMF_LEAVE_ON     2     // Continue transmitting same tone
#define STEP_DTMF_TURN_OFF     3     // Stop transmitting (enter silence)
#define STEP_DTMF_LEAVE_OFF    4     // Continue silence
#define STEP_DTMF_CHANGE_FREQ  5     // Change to next digit frequency
#define STEP_DTMF_CYCLE_END    6     // End of sequence - release generators

class AsyncDTMF {
private:
    const char* _digit_sequence;
    int _sequence_length;
    int _current_digit_index;
    
    // DTMF state machine
    enum DTMFState {
        DTMF_IDLE,
        DTMF_PLAYING_TONE,
        DTMF_SILENCE,
        DTMF_INTER_DIGIT_GAP,
        DTMF_SEQUENCE_COMPLETE
    };
    
    DTMFState _dtmf_state;
    unsigned long _next_event_time;
    bool _transmitting;
    bool _active;
    
    // Human timing calculation helpers
    unsigned long calculateToneDuration();      // Variable tone hold time
    unsigned long calculateSilenceDuration();   // Variable inter-tone silence
    unsigned long calculateDigitGap(int current_position);  // Context-aware digit gaps based on current position

public:
    AsyncDTMF();
    
    // Initialize with digit sequence
    void start_dtmf_transmission(const char* sequence, bool repeating = true);
    
    // Call periodically to advance state machine
    // Returns STEP_DTMF_* constants indicating what action to take
    int step_dtmf(unsigned long time);
    
    // Get current digit for frequency calculation
    char get_current_digit() const;
    
    // Check if currently transmitting a tone
    bool is_transmitting() const { return _transmitting; }
    
    // Reset sequence to beginning
    void reset_sequence();
};

#endif
