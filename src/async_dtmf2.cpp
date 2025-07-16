#include <Arduino.h>
#include <string.h>
#include "../include/async_dtmf2.h"

AsyncDTMF2::AsyncDTMF2() {
    _digit_sequence = nullptr;
    _sequence_length = 0;
    _current_digit_index = 0;
    _dtmf_state = DTMF_IDLE;
    _next_event_time = 0;
    _transmitting = false;
    _active = false;
}

void AsyncDTMF2::start_dtmf_transmission(const char* sequence, bool repeating) {
    _digit_sequence = sequence;
    _sequence_length = strlen(sequence);
    _current_digit_index = 0;
    _dtmf_state = DTMF_IDLE;
    _next_event_time = 0;
    _transmitting = false;
    _active = true;
}

// Temporary compatibility method for telco mode (will be removed)
void AsyncDTMF2::configure_timing(int type) {
    // No-op for now - keeping for build compatibility
}

void AsyncDTMF2::start_telco_transmission(bool repeat) {
    // No-op for now - keeping for build compatibility  
}

int AsyncDTMF2::step_telco(unsigned long time) {
    return step_dtmf(time);  // Temporary compatibility - delegate to DTMF
}

int AsyncDTMF2::step_dtmf(unsigned long time) {
    if (!_active) {
        return STEP_DTMF_LEAVE_OFF;
    }
    
    if (time < _next_event_time) {
        // Not time for state change yet
        return _transmitting ? STEP_DTMF_LEAVE_ON : STEP_DTMF_LEAVE_OFF;
    }
    
    // Time for state transition
    switch(_dtmf_state) {
        case DTMF_IDLE:
            if(_current_digit_index < _sequence_length) {
                _dtmf_state = DTMF_PLAYING_TONE;
                _transmitting = true;
                _next_event_time = time + DTMF_TONE_DURATION;
                return STEP_DTMF_TURN_ON;  // Start transmitting new digit
            } else {
                _dtmf_state = DTMF_SEQUENCE_COMPLETE;
                _transmitting = false;
                _next_event_time = time + DTMF_SEQUENCE_GAP;
                return STEP_DTMF_CYCLE_END;  // End of sequence - release generators
            }
            break;
            
        case DTMF_PLAYING_TONE:
            _transmitting = false;
            _dtmf_state = DTMF_SILENCE;
            _next_event_time = time + DTMF_SILENCE_DURATION;
            return STEP_DTMF_TURN_OFF;  // Stop transmitting (enter silence)
            break;
            
        case DTMF_SILENCE:
            _current_digit_index++;
            if(_current_digit_index < _sequence_length) {
                // Check if we need a longer gap (between digit groups)
                char current_char = _digit_sequence[_current_digit_index - 1];
                char next_char = _digit_sequence[_current_digit_index];
                
                // Add longer gap after certain characters (like area code separator)
                if(current_char == '1' || next_char == '-' || next_char == ' ') {
                    _dtmf_state = DTMF_INTER_DIGIT_GAP;
                    _next_event_time = time + DTMF_DIGIT_GAP;
                } else {
                    _dtmf_state = DTMF_IDLE;
                    _next_event_time = time + 50;  // Short gap between normal digits
                }
            } else {
                _dtmf_state = DTMF_SEQUENCE_COMPLETE;
                _next_event_time = time + DTMF_SEQUENCE_GAP;
                return STEP_DTMF_CYCLE_END;  // End of sequence
            }
            return STEP_DTMF_LEAVE_OFF;  // Continue silence
            break;
            
        case DTMF_INTER_DIGIT_GAP:
            _dtmf_state = DTMF_IDLE;
            _next_event_time = time + 50;
            return STEP_DTMF_LEAVE_OFF;  // Continue silence
            break;
            
        case DTMF_SEQUENCE_COMPLETE:
            // First time reaching complete state - signal cycle end
            _active = false;  // Stop responding to step_dtmf calls
            _transmitting = false;
            return STEP_DTMF_CYCLE_END;  // Tell SimDTMF2 to release generators and wait
            break;
    }
    
    return STEP_DTMF_LEAVE_OFF;
}

char AsyncDTMF2::get_current_digit() const {
    if(_current_digit_index < _sequence_length && _digit_sequence) {
        return _digit_sequence[_current_digit_index];
    }
    return '0';  // Default digit if invalid
}

void AsyncDTMF2::reset_sequence() {
    _current_digit_index = 0;
    _dtmf_state = DTMF_IDLE;
    _transmitting = false;
}
