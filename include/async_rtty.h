#ifndef __ASYNC_RTTY_H__
#define __ASYNC_RTTY_H__

#include "async_modulator.h"
#include <Arduino.h>

#define RTTY_TIME_BASIS 22
#define RTTY_TIME_BASIS2 33

// RTTY-specific step codes (inherit common ones from base class)
#define STEP_RTTY_TURN_ON   STEP_TURN_ON
#define STEP_RTTY_TURN_OFF  STEP_TURN_OFF
#define STEP_RTTY_LEAVE_ON  STEP_LEAVE_ON
#define STEP_RTTY_LEAVE_OFF STEP_LEAVE_OFF

// Baudot RTTY code definitions
#define BAUDOT_LTRS    0x1F  // Switch to letters mode
#define BAUDOT_FIGS    0x1B  // Switch to figures mode  
#define BAUDOT_SPACE   0x04  // Space character
#define BAUDOT_CR      0x08  // Carriage return
#define BAUDOT_LF      0x02  // Line feed

class AsyncRTTY : public AsyncModulator
{
public:
    AsyncRTTY();

    // Implement AsyncModulator interface
    virtual void start_transmission(const char* message, int timing_param) override;
    virtual int step_modulator(unsigned long time) override;
    virtual bool is_transmission_complete() const override;
    
    // RTTY-specific interface (for backward compatibility)
    void start_rtty_message(const char* message, bool repeat);
    int step_rtty(unsigned long time) { return step_modulator(time); }
    bool is_message_complete() const { return is_transmission_complete(); }
    
private:private:
    bool start_step_element(unsigned long time);
    unsigned long compute_element_time(unsigned long time, bool stop_bit);
    int step_element(unsigned long time);
    unsigned char get_baudot_code(char c);  // New method to get Baudot code for character

    // RTTY-specific state variables
    int async_str_pos;      // Current position in string  
    bool async_repeat;      // Whether to repeat the message
    byte async_phase;       // Current transmission phase
    char async_char;        // Current character being transmitted
    byte async_rtty;        // Current bit pattern for character
    bool async_space;       // True when in gap between elements
};
// JH!

// extern void start_rtty(const char *s, int wpm, bool repeat);
// extern int step_rtty(unsigned long time);

#endif
