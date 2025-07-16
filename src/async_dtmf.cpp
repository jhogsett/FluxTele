#include "async_dtmf.h"
#include <string.h>

AsyncDTMF::AsyncDTMF() {
    _digit_sequence = nullptr;
    _sequence_length = 0;
    _current_digit_index = 0;
    _dtmf_state = DTMF_IDLE;
    _next_event_time = 0;
    _transmitting = false;
    _active = false;
}

void AsyncDTMF::start_dtmf_transmission(const char* sequence, bool repeating) {
    _digit_sequence = sequence;
    _sequence_length = strlen(sequence);
    _current_digit_index = 0;
    _dtmf_state = DTMF_IDLE;
    _next_event_time = 0;
    _transmitting = false;
    _active = true;
}

int AsyncDTMF::step_dtmf(unsigned long time) {
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
            return STEP_DTMF_CYCLE_END;  // Tell SimDTMF to release generators and wait
            break;
    }
    
    return STEP_DTMF_LEAVE_OFF;
}

char AsyncDTMF::get_current_digit() const {
    if(_current_digit_index < _sequence_length && _digit_sequence) {
        return _digit_sequence[_current_digit_index];
    }
    return '0';  // Default digit if invalid
}

void AsyncDTMF::reset_sequence() {
    _current_digit_index = 0;
    _dtmf_state = DTMF_IDLE;
    _transmitting = false;
}
