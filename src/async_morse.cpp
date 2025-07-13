#include <Arduino.h>

#include "../include/async_morse.h"
#include "../include/eeprom_tables.h"

// ========================================
// MORSE CODE LOOKUP TABLE  
// ========================================
// Each byte represents a morse character with bits indicating dot (0) or dash (1)
// Bits are read from LSB to MSB, with unused high bits as padding

#ifndef USE_EEPROM_TABLES
// Traditional Flash-based lookup table (default)
const unsigned char morsedata[] PROGMEM = {
    0b10100000, // A  .-
    0b00011000, // B  -...
    0b01011000, // C  -.-.
    0b00110000, // D  -..
    0b01000000, // E  .
    0b01001000, // F  ..-.
    0b01110000, // G  --.
    0b00001000, // H  ....
    0b00100000, // I  ..
    0b11101000, // J  .---
    0b10110000, // K  -.-
    0b00101000, // L  .-..
    0b11100000, // M  --
    0b01100000, // N  -.
    0b11110000, // O  ---
    0b01101000, // P  .--.
    0b10111000, // Q  --.-
    0b01010000, // R  .-.
    0b00010000, // S  ...
    0b11000000, // T  -
    0b10010000, // U  ..-
    0b10001000, // V  ...-
    0b11010000, // W  .--
    0b10011000, // X  -..-
    0b11011000, // Y  -.--
    0b00111000, // Z  --..
    0b11111100, // 0  -----
    0b11110100, // 1  .----
    0b11100100, // 2  ..---
    0b11000100, // 3  ...--
    0b10000100, // 4  ....-
    0b00000100, // 5  .....
    0b00001100, // 6  -....
    0b00011100, // 7  --...
    0b00111100, // 8  ---..
    0b01111100  // 9  ----.
};
#endif // USE_EEPROM_TABLES

// ========================================
// MORSE DATA ACCESS HELPER
// ========================================
unsigned char get_morse_data(int index) {
#ifdef USE_EEPROM_TABLES
    // Use EEPROM-based lookup (slower but saves Flash)
    return eeprom_read_morse_data(index);
#else
    // Use traditional Flash-based lookup (faster)
    if (index < 0 || index >= MORSE_TABLE_SIZE) return 0;
    return pgm_read_byte(morsedata + index);
#endif
}

// ========================================
// CONSTRUCTOR
// ========================================
AsyncMorse::AsyncMorse() : AsyncModulator() {
    // Initialize Morse-specific state variables
    async_phase = PHASE_DONE;
    async_char = 0;
    async_morse = 0;
    async_space = false;
    async_just_completed = false;
    _fist_quality = 0;  // Default to perfect fist (mechanical precision)
}

// ========================================
// CHARACTER LOOKUP HELPER
// ========================================    
char AsyncMorse::lookup_morse_char(char c){
    // Convert character to morse table index
    // Returns: 0-25 for A-Z, 26-35 for 0-9, 0 for invalid characters
    int offset = -1;
    if(c >= '0' && c <= 'z'){
        if(c >= '0' && c <= '9'){
            c -= '0';
            offset = 26;  // Numbers start at index 26
        } else if(c >= 'A' && c <= 'Z'){
            c -= 'A';
            offset = 0;   // Letters start at index 0
        } else if(c >= 'a' && c <= 'z'){
            c -= 'a';     // Convert lowercase to uppercase
            offset = 0;   // Letters start at index 0
        }
    }
    if(offset >= 0)
        return c + offset;
    else
        return 0;  // Invalid character
}

// ========================================
// ELEMENT INITIALIZATION HELPER
// ========================================
// returns true unless the start bit is not found
bool AsyncMorse::start_step_element(unsigned long time){
    set_element_done(false);
    async_morse = get_morse_data(async_char);
    
    for(int element = 0; element < 7; element++){
        async_morse = async_morse >> 1;
        byte bit = async_morse & 0x1;
        if(bit == 1){
            // Set the element counter to where we found the start bit
            set_current_element(element);
            // Set next event to start immediately (time + 0) to eliminate startup delay
            set_next_event_time(time);
            async_space = true;
            return true;
        }
    }
    return false;
}

void AsyncMorse::start_transmission(const char *s, int wpm){
    set_string(s);
    set_element_delay(MORSE_TIME_FROM_WPM(wpm));

    async_phase = PHASE_CHAR;
    set_string_position(0);
    async_morse = 0;    
    set_current_element(0);
    set_active(false);
    set_next_event_time(0L);
    set_switched_on(false);  // Reset to ensure TURN_ON event is generated
    async_space = false;
    async_just_completed = false;  // Initialize completion flag

    set_element_done(true);

    async_char = get_current_char();
    
    if(async_char == ' '){
        async_phase = PHASE_SPACE;
        return;
    }

    async_char = lookup_morse_char(async_char);        // Initialize with time 0 to eliminate startup delay
    if(!start_step_element(0))
        return;

    if(async_char > 0){
        async_phase = PHASE_CHAR;
    }    
}

unsigned long AsyncMorse::compute_element_time(unsigned long time, byte element_count, bool is_space){
    unsigned long base_time = element_count * get_element_delay();
    
    // Apply fist quality variations if enabled (0 = perfect, 255 = maximum bad fist)
    if (_fist_quality > 0) {
        // Very conservative timing variations for authentic but stable CW
        // Even "bad fist" operators don't vary timing by more than a few percent
        
        // Maximum variation: ±3% at maximum bad fist (255)
        // This gives realistic human timing variation without breaking modulation
        int max_variation_percent = (_fist_quality * 3) / 255;
        
        // Even more conservative approach: cap at 2% maximum
        if (max_variation_percent > 2) max_variation_percent = 2;
        
        // Different timing errors for elements vs spacing
        int variation_percent;
        if (is_space) {
            // Inter-element and inter-character spacing gets slightly more variation
            variation_percent = max_variation_percent + 1;  // Just +1% more
            if (variation_percent > 3) variation_percent = 3;  // Hard cap at 3%
        } else {
            // Element timing (dots and dashes) has smaller variations
            variation_percent = max_variation_percent;
            if (variation_percent > 2) variation_percent = 2;  // Hard cap at 2%
        }
        
        // Generate random variation (both positive and negative)
        if (variation_percent > 0) {
            int random_variation = (random(variation_percent * 2 + 1)) - variation_percent;
            // Apply the variation more safely
            long variation = ((long)base_time * random_variation) / 100;
            long new_time = (long)base_time + variation;
            
            // Safety limits: ensure timing stays within reasonable bounds
            long min_time = (long)base_time * 80 / 100;  // No less than 80% of base time
            long max_time = (long)base_time * 120 / 100; // No more than 120% of base time
            
            if (new_time < min_time) {
                new_time = min_time;
            } else if (new_time > max_time) {
                new_time = max_time;
            }
            
            base_time = (unsigned long)new_time;
        }
    }
    
    return time + base_time;
}

int AsyncMorse::step_element(unsigned long time){
    if(async_phase != PHASE_CHAR)
        return STEP_ELEMENT_DONE;

    if(is_element_done()){
        return STEP_ELEMENT_DONE;
    }    if(!is_time_ready(time)){
        return STEP_ELEMENT_EARLY;
    }
      if(async_space){
        async_space = false;
        advance_element();

        if(get_current_element() >= 7){
            set_element_done(true);
            return STEP_ELEMENT_DONE;        }

        async_morse = async_morse >> 1;
        byte bit = async_morse & 0x1;
    
        set_active(true);

        set_next_event_time(compute_element_time(time, bit == 1 ? 3 : 1, false));
    } else {
        async_space = true;
        set_active(false);
        set_next_event_time(compute_element_time(time, 1, false));//time + async_element_del;
    }

    return STEP_ELEMENT_ACTIVE;
}

// returns false if past the end of the sending string
bool AsyncMorse::step_position(unsigned long time){
    int ret = step_element(time);     if(ret == STEP_ELEMENT_EARLY)
        return true;

    if(ret == STEP_ELEMENT_DONE){
        advance_string_position();
        if(at_string_end())
            return false;
        
        async_char = get_current_char();

        if(async_char == ' '){
            advance_string_position();
            async_char = get_current_char();

            if(async_char != ' '){
                async_char = lookup_morse_char(async_char);
                if(!start_step_element(time)){
                    return false;
                }
            }

            async_phase = PHASE_SPACE;
            set_next_event_time(compute_element_time(time, 7, true));//time + (7 * async_element_del);

            return true;
        }        async_char = lookup_morse_char(async_char);

        if(!start_step_element(time))
            return false;

        set_next_event_time(compute_element_time(time, 3, true));//time + (3 * async_element_del);
        async_phase = PHASE_SPACE;
    }
    return true;
}

void AsyncMorse::step_space(unsigned long time){
    if(!is_time_ready(time)){
        return;
    }    async_phase = PHASE_CHAR;
}

// ========================================
// TRANSMISSION COMPLETION HELPER
// ========================================
void AsyncMorse::handle_transmission_complete(unsigned long time) {
    // Mark message as complete and stop transmission
    async_just_completed = true;
    async_phase = PHASE_DONE;
}

int AsyncMorse::step_modulator(unsigned long time){
    switch(async_phase){
        case PHASE_DONE:
            break;        case PHASE_CHAR:
            if(!step_position(time)){
                handle_transmission_complete(time);            }
            break;
        case PHASE_SPACE:
            step_space(time);
            break;    }
    
    // Check for message completion before normal wave generator control
    if(async_just_completed) {
        async_just_completed = false;  // Reset flag after reporting
        return STEP_MORSE_MESSAGE_COMPLETE;
    }
    
    // Use base class output generation
    return generate_output_step();
}

// ========================================
// COMPLETION CHECK
// ========================================
bool AsyncMorse::is_transmission_complete() const {
    return async_phase == PHASE_DONE;
}


