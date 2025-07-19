#ifndef __ASYNC_DTMF_H__
#define __ASYNC_DTMF_H__

// AsyncDTMF - DTMF sequence timing manager (similar to AsyncTelco)
// Handles the timing and state transitions for DTMF digit sequences

// DTMF timing constants
#define DTMF_TONE_DURATION     150    // 150ms tone duration
#define DTMF_SILENCE_DURATION  150    // 150ms silence between tones
#define DTMF_DIGIT_GAP         350    // 350ms gap between digit groups
#define DTMF_SEQUENCE_GAP      3000   // 3 second gap between sequence repeats

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
