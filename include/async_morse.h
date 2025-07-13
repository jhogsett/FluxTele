#ifndef __ASYNC_MORSE_H__
#define __ASYNC_MORSE_H__

#include "async_modulator.h"

#define MORSE_TIME_FROM_WPM(w) (1000 / w)

#define PHASE_DONE 0
#define PHASE_CHAR 1
#define PHASE_SPACE 2

#define MAX_ELEMENT 6

// Morse-specific step codes (inherit common ones from base class)
#define STEP_MORSE_TURN_ON   STEP_TURN_ON
#define STEP_MORSE_TURN_OFF  STEP_TURN_OFF
#define STEP_MORSE_LEAVE_ON  STEP_LEAVE_ON
#define STEP_MORSE_LEAVE_OFF STEP_LEAVE_OFF
#define STEP_MORSE_MESSAGE_COMPLETE STEP_MESSAGE_COMPLETE

class AsyncMorse : public AsyncModulator
{
public:
    AsyncMorse();
    
    // Implement AsyncModulator interface
    virtual void start_transmission(const char* text, int wpm) override;
    virtual int step_modulator(unsigned long time) override;
    virtual bool is_transmission_complete() const override;
    
    // Morse-specific interface (for backward compatibility)
    void start_morse(const char *s, int wpm) { start_transmission(s, wpm); }
    int step_morse(unsigned long time) { return step_modulator(time); }
    bool is_done() const { return is_transmission_complete(); }
    
    // Operator fist quality control
    void set_fist_quality(byte quality) { _fist_quality = quality; }
    byte get_fist_quality() const { return _fist_quality; }
    
private:// ========================================
    // INTERNAL HELPER METHODS
    // ========================================
    char lookup_morse_char(char c);    bool start_step_element(unsigned long time);
    unsigned long compute_element_time(unsigned long time, byte element_count, bool is_space);
    int step_element(unsigned long time);
    bool step_position(unsigned long time);
    void step_space(unsigned long time);
    void handle_transmission_complete(unsigned long time);
    
    // ========================================
    // STATE VARIABLES (Morse-specific)
    // ========================================
      
    // Morse-specific state  
    byte async_phase;                      // Current phase: PHASE_DONE, PHASE_CHAR, PHASE_SPACE
    char async_char;                       // Morse table index for current character
    byte async_morse;                      // Bit pattern for current character
    
    // Timing state (some inherited from base class)
    bool async_space;                      // True when in gap between elements
    bool async_just_completed;             // True for one step after message completion
    
    // Operator fist quality (0 = perfect, 255 = maximum bad fist)
    byte _fist_quality;                    // Controls timing variations for realistic operator simulation

};

#endif
