#include "../include/async_modulator.h"

// ========================================
// CONSTRUCTOR
// ========================================
AsyncModulator::AsyncModulator() {
    // Initialize all common state variables to safe defaults
    async_str = NULL;
    async_length = 0;
    async_position = 0;
    async_element_del = 0;
    async_element = 0;
    async_element_done = true;
    async_next_event = 0L;
    async_active = false;
    async_switched_on = false;
}

// ========================================
// COMMON TIMING HELPERS
// ========================================
bool AsyncModulator::is_time_ready(unsigned long current_time) const {
    return current_time >= async_next_event;
}

void AsyncModulator::set_next_event_time(unsigned long time) {
    async_next_event = time;
}

// ========================================
// COMMON OUTPUT CONTROL
// ========================================
int AsyncModulator::generate_output_step() {
    // Generate output signal based on current active state
    if(async_active != async_switched_on) {
        async_switched_on = async_active;
        return async_active ? STEP_TURN_ON : STEP_TURN_OFF;
    } else {
        return async_active ? STEP_LEAVE_ON : STEP_LEAVE_OFF;
    }
}

// ========================================
// COMMON STRING HANDLING
// ========================================
void AsyncModulator::set_string(const char* str) {
    async_str = str;
    async_length = str ? strlen(str) : 0;
    async_position = 0;
}

char AsyncModulator::get_current_char() const {
    if (async_str && async_position < async_length) {
        return async_str[async_position];
    }
    return '\0';
}
