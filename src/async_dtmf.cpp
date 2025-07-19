#include "async_dtmf.h"
#include <string.h>
#include <Arduino.h>

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
                _next_event_time = time + calculateToneDuration();  // Variable human timing
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
            _next_event_time = time + calculateSilenceDuration();  // Variable inter-tone silence
            return STEP_DTMF_TURN_OFF;  // Stop transmitting (enter silence)
            break;
            
        case DTMF_SILENCE:
            _current_digit_index++;
            if(_current_digit_index < _sequence_length) {
                // Use human-like timing with positional context awareness
                unsigned long gap_time = calculateDigitGap(_current_digit_index);
                
                _dtmf_state = DTMF_INTER_DIGIT_GAP;
                _next_event_time = time + gap_time;
            } else {
                _dtmf_state = DTMF_SEQUENCE_COMPLETE;
                _next_event_time = time + DTMF_SEQUENCE_GAP;
                return STEP_DTMF_CYCLE_END;  // End of sequence
            }
            return STEP_DTMF_LEAVE_OFF;  // Continue silence
            break;
            
        case DTMF_INTER_DIGIT_GAP:
            _dtmf_state = DTMF_IDLE;
            return STEP_DTMF_LEAVE_OFF;  // Continue silence, will start next digit on next call
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

// Human timing calculation helpers for realistic touch-tone dialing behavior
unsigned long AsyncDTMF::calculateToneDuration() {
    // Humans hold buttons for variable amounts of time (200-400ms)
    return DTMF_TONE_MIN_DURATION + random(DTMF_TONE_MAX_DURATION - DTMF_TONE_MIN_DURATION);
}

unsigned long AsyncDTMF::calculateSilenceDuration() {
    // Variable silence between digit tones (100-200ms)
    return DTMF_SILENCE_MIN_DURATION + random(DTMF_SILENCE_MAX_DURATION - DTMF_SILENCE_MIN_DURATION);
}

unsigned long AsyncDTMF::calculateDigitGap(int current_position) {
    // Context-aware gaps based on human dialing patterns
    int previous_position = current_position - 1;
    
    // Fast dialing for repeated digits (like "00", "555", "99")
    if (current_position > 0 && _digit_sequence && 
        _digit_sequence[current_position] == _digit_sequence[previous_position]) {
        return DTMF_DIGIT_GAP_MIN + random(100);  // 200-300ms for repeated digits
    }
    
    // Longer thinking pauses at natural break points based on position just completed
    if (previous_position == 0 ||          // After country code digit (position 0: "1")
        previous_position == 3 ||          // After area code (position 3: last digit of "555")  
        previous_position == 6) {          // After exchange prefix (position 6: last digit of "123")
        return (DTMF_DIGIT_GAP_MIN + DTMF_DIGIT_GAP_MAX) / 2 + random(200);  // 500-700ms thinking pause
    }
    
    // Default inter-digit gap with natural variation
    return DTMF_DIGIT_GAP_MIN + random(DTMF_DIGIT_GAP_MAX - DTMF_DIGIT_GAP_MIN);  // 200-800ms
}
