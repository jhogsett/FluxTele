#ifndef __ASYNC_MODULATOR_H__
#define __ASYNC_MODULATOR_H__

#include <Arduino.h>

// Common step return codes for all modulators
#define STEP_TURN_ON   1
#define STEP_TURN_OFF  2
#define STEP_LEAVE_ON  3
#define STEP_LEAVE_OFF 4
#define STEP_MESSAGE_COMPLETE 5  // Signals end of message transmission

#define STEP_ELEMENT_EARLY 0
#define STEP_ELEMENT_ACTIVE 1
#define STEP_ELEMENT_DONE 2

/**
 * Base class for asynchronous modulators (Morse, RTTY, etc.)
 * Provides common timing, state management, and output control functionality
 * that is shared between different modulation types.
 */
class AsyncModulator
{
public:
    AsyncModulator();
    virtual ~AsyncModulator() {}
      // Pure virtual methods that must be implemented by derived classes
    virtual void start_transmission(const char* text, int timing_param) = 0;
    virtual int step_modulator(unsigned long time) = 0;
    virtual bool is_transmission_complete() const = 0;
    
    // ========================================
    // DEBUG ACCESS
    // ========================================
    void set_switched_on(bool switched_on) { async_switched_on = switched_on; }
    bool is_switched_on() const { return async_switched_on; }
    
protected:    // ========================================
    // COMMON TIMING HELPERS
    // ========================================
    bool is_time_ready(unsigned long current_time) const;
    void set_next_event_time(unsigned long time);
    unsigned long get_next_event_time() const { return async_next_event; }
    
    // ========================================
    // COMMON OUTPUT CONTROL
    // ========================================
    int generate_output_step();  // Returns appropriate STEP_* code based on active state
    void set_active(bool active) { async_active = active; }
    bool is_active() const { return async_active; }
    
    // ========================================
    // COMMON STRING HANDLING
    // ========================================
    void set_string(const char* str);
    const char* get_string() const { return async_str; }
    int get_string_length() const { return async_length; }
    int get_string_position() const { return async_position; }
    void set_string_position(int pos) { async_position = pos; }
    void advance_string_position() { async_position++; }
    char get_current_char() const;
    bool at_string_end() const { return async_position >= async_length; }
    
    // ========================================
    // COMMON TIMING CONFIGURATION
    // ========================================
    void set_element_delay(int delay) { async_element_del = delay; }
    int get_element_delay() const { return async_element_del; }
    
    // ========================================
    // COMMON ELEMENT STATE
    // ========================================
    void set_element_done(bool done) { async_element_done = done; }
    bool is_element_done() const { return async_element_done; }
    byte get_current_element() const { return async_element; }
    void set_current_element(byte element) { async_element = element; }
    void advance_element() { async_element++; }
    
private:
    // ========================================
    // COMMON STATE VARIABLES
    // ========================================
    
    // Text and position management
    const char *async_str;           // String being transmitted
    int async_length;                // Length of text string
    int async_position;              // Current position in text string
    int async_element_del;           // Base timing unit in milliseconds
    
    // Element timing and state
    byte async_element;              // Current element within character
    bool async_element_done;         // True when current element is finished
    unsigned long async_next_event;  // Time when next state change occurs
    
    // Output state tracking
    bool async_active;               // True when transmitter should be ON
    bool async_switched_on;          // Tracks output transitions for TURN_ON/TURN_OFF
};

#endif // __ASYNC_MODULATOR_H__
