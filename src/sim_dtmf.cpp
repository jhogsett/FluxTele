#include "station_config.h"
#include "sim_dtmf.h"
#include "wavegen.h"
#include "vfo.h"
#include "saved_data.h"
#include "signal_meter.h"

// DTMF frequency lookup tables (based on reference call_sequence.ino)
const float SimDTMF::ROW_FREQUENCIES[4] = {
    DTMF_ROW_1,  // 697.0 Hz
    DTMF_ROW_2,  // 770.0 Hz  
    DTMF_ROW_3,  // 852.0 Hz
    DTMF_ROW_4   // 941.0 Hz
};

const float SimDTMF::COL_FREQUENCIES[4] = {
    DTMF_COL_1,  // 1209.0 Hz
    DTMF_COL_2,  // 1336.0 Hz
    DTMF_COL_3,  // 1477.0 Hz
    DTMF_COL_4   // 1633.0 Hz
};

// DTMF digit to row mapping (from reference call_sequence.ino)
const int SimDTMF::DIGIT_TO_ROW[16] = {
    3,  // 0 -> ROW4
    0,  // 1 -> ROW1
    0,  // 2 -> ROW1  
    0,  // 3 -> ROW1
    1,  // 4 -> ROW2
    1,  // 5 -> ROW2
    1,  // 6 -> ROW2
    2,  // 7 -> ROW3
    2,  // 8 -> ROW3
    2,  // 9 -> ROW3
    3,  // * -> ROW4
    3,  // # -> ROW4
    0,  // A -> ROW1
    1,  // B -> ROW2
    2,  // C -> ROW3
    3   // D -> ROW4
};

// DTMF digit to column mapping (from reference call_sequence.ino)
const int SimDTMF::DIGIT_TO_COL[16] = {
    1,  // 0 -> COL2
    0,  // 1 -> COL1
    1,  // 2 -> COL2
    2,  // 3 -> COL3
    0,  // 4 -> COL1
    1,  // 5 -> COL2
    2,  // 6 -> COL3
    0,  // 7 -> COL1
    1,  // 8 -> COL2
    2,  // 9 -> COL3
    0,  // * -> COL1
    2,  // # -> COL3
    3,  // A -> COL4
    3,  // B -> COL4
    3,  // C -> COL4
    3   // D -> COL4
};

// Constructor for fixed phone number
SimDTMF::SimDTMF(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, const char* sequence)
    : SimDualTone(wave_gen_pool, fixed_freq), _signal_meter(signal_meter), _digit_sequence(sequence)
{
    _current_row_freq = 0.0f;
    _current_col_freq = 0.0f;
    _use_random_numbers = false;  // Using fixed sequence
    
    // Initialize wait delay management (matching SimTelco pattern)
    _in_wait_delay = false;
    _next_cycle_time = 0;
}

// Constructor for random phone number generation
SimDTMF::SimDTMF(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq)
    : SimDualTone(wave_gen_pool, fixed_freq), _signal_meter(signal_meter), _digit_sequence(nullptr)
{
    _current_row_freq = 0.0f;
    _current_col_freq = 0.0f;
    _use_random_numbers = true;   // Using random generation
    
    // Generate initial random phone number
    generate_random_nanp_number();
    _digit_sequence = _generated_number;  // Point to generated number
    
    // Initialize wait delay management (matching SimTelco pattern)
    _in_wait_delay = false;
    _next_cycle_time = 0;
}

bool SimDTMF::begin(unsigned long time) {
    // Attempt to acquire all required realizers atomically
    if(!common_begin(time, _fixed_freq)) {
        return false;  // Failed to acquire all needed wave generators
    }

    // Initialize all acquired wave generators to silent
    int realizer_index = 0;
    
    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
        wavegen_a->set_frequency(SILENT_FREQ, false);
    }

    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->set_frequency(SILENT_FREQ, false);
    }

    // Set enabled and force frequency update
    _enabled = true;
    force_frequency_update();
    realize();  // CRITICAL: Set active state for audio output!
    
    // Debug output for phone number (only when starting new sequence)
    if (_use_random_numbers) {
        Serial.print("DTMF: Dialing random number: ");
        Serial.println(_generated_number);
    }
    
    // Start AsyncDTMF sequence (matching SimTelco pattern with AsyncTelco)
    _dtmf.start_dtmf_transmission(_digit_sequence, true);
    _in_wait_delay = false;  // Clear wait delay state
    
    return true;
}

bool SimDTMF::update(Mode *mode) {
    common_frequency_update(mode);

    if(_enabled && has_all_realizers()) {
        // Update frequencies for all acquired wave generators
        int realizer_index = 0;

        int realizer_a = get_realizer(realizer_index++);
        if(realizer_a != -1) {
            WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
            wavegen_a->set_frequency(_frequency);
        }

        int realizer_c = get_realizer(realizer_index++);
        if(realizer_c != -1) {
            WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
            wavegen_c->set_frequency(_frequency2);
        }
    }

    realize();
    return true;
}

bool SimDTMF::step(unsigned long time) {
    // Handle DTMF sequence timing using AsyncDTMF (matching SimTelco pattern)
    int dtmf_state = _dtmf.step_dtmf(time);
    
    switch(dtmf_state) {
        case STEP_DTMF_TURN_ON:
            // New digit starting - set frequencies and activate
            set_digit_frequencies(_dtmf.get_current_digit());
            force_frequency_update();
            _active = true;
            realize();
            send_carrier_charge_pulse(_signal_meter);
            break;
            
        case STEP_DTMF_LEAVE_ON:
            // Carrier remains on - send another charge pulse
            send_carrier_charge_pulse(_signal_meter);
            break;
            
        case STEP_DTMF_TURN_OFF:
            // Enter silence - set silent frequencies but keep generators
            _current_row_freq = 0.0f;
            _current_col_freq = 0.0f;
            force_frequency_update();
            // Note: Keep _active = true to maintain generator allocation
            break;
            
        case STEP_DTMF_LEAVE_OFF:
            // Remain in silence - no action needed
            break;
            
        case STEP_DTMF_CYCLE_END:
            // End of sequence - properly release wave generators using end()
            end();  // This calls SimDualTone::end() -> Realization::end() to free realizers
            _in_wait_delay = true;
            _next_cycle_time = time + 3000;  // 3 second pause
            break;
    }
    
    // Check if it's time to restart sequence after wait delay (matching SimTelco pattern)
    if(_in_wait_delay && time >= _next_cycle_time) {
        // DYNAMIC PIPELINING: Try to reallocate WaveGen for next sequence cycle
        if(begin(time)) {  // Only proceed if WaveGen is available
            _in_wait_delay = false;
        } else {
            // WaveGen not available - extend wait period and try again later
            _next_cycle_time = time + 500 + random(1000);     // Try again in 0.5-1.5 seconds
        }
    }
    
    return true;
}

void SimDTMF::realize() {
    if(!has_all_realizers()) {
        return;  // No WaveGens allocated
    }

    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }

    // Set active state for all acquired wave generators
    int realizer_index = 0;

    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
        wavegen_a->set_active_frequency(_active);
    }

    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->set_active_frequency(_active);
    }
}

void SimDTMF::set_digit_frequencies(char digit) {
    int digit_index = char_to_digit_index(digit);
    
    if(digit_index >= 0 && digit_index < 16) {
        int row_index = DIGIT_TO_ROW[digit_index];
        int col_index = DIGIT_TO_COL[digit_index];
        
        _current_row_freq = ROW_FREQUENCIES[row_index];
        _current_col_freq = COL_FREQUENCIES[col_index];
    } else {
        // Invalid character - use silence
        _current_row_freq = 0.0f;
        _current_col_freq = 0.0f;
    }
}

int SimDTMF::char_to_digit_index(char c) {
    switch(c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case '*': return 10;
        case '#': return 11;
        case 'A': case 'a': return 12;
        case 'B': case 'b': return 13;
        case 'C': case 'c': return 14;
        case 'D': case 'd': return 15;
        default: return -1;  // Invalid character
    }
}

void SimDTMF::randomize() {
    // CRITICAL: Ensure proper cleanup when station gets moved by StationManager
    // This prevents wave generators from getting "stuck on" during station moves
    end();  // Release all wave generators before randomizing
    
    // Generate new random phone number if using random generation
    if (_use_random_numbers) {
        generate_random_nanp_number();
        _digit_sequence = _generated_number;  // Update pointer to new number
        Serial.print("DTMF: Generated new random number: ");
        Serial.println(_generated_number);
    }
    
    // Reset AsyncDTMF sequence
    _dtmf.reset_sequence();
    
    // Reset wait delay state
    _in_wait_delay = false;
    _next_cycle_time = 0;
}

void SimDTMF::generate_random_nanp_number() {
    // Generate authentic North American Numbering Plan (NANP) phone number
    // Format: 1 + NXX + NXX + XXXX (11 digits total)
    // Where N = 2-9, X = 0-9
    
    // Country code (always 1 for NANP)
    char country_code = '1';
    
    // Area code: NXX format (first digit 2-9, others 0-9)
    // Use realistic area codes with geographic diversity
    int realistic_area_codes[] = {
        212, 213, 214, 215, 216, 217, 301, 302, 303, 304, 305, 307, 309,
        312, 313, 314, 315, 316, 317, 318, 319, 401, 402, 403, 404, 405,
        406, 407, 408, 409, 410, 412, 413, 414, 415, 416, 417, 418, 419,
        501, 502, 503, 504, 505, 507, 508, 509, 510, 512, 513, 514, 515,
        516, 517, 518, 519, 601, 602, 603, 604, 605, 606, 607, 608, 609,
        610, 612, 613, 614, 615, 616, 617, 618, 619, 701, 702, 703, 704,
        705, 706, 707, 708, 709, 712, 713, 714, 715, 716, 717, 718, 719,
        801, 802, 803, 804, 805, 806, 807, 808, 809, 810, 812, 813, 814,
        815, 816, 817, 818, 819, 901, 902, 903, 904, 905, 906, 907, 908,
        909, 910, 912, 913, 914, 915, 916, 917, 918, 919
    };
    
    int area_code = realistic_area_codes[random(sizeof(realistic_area_codes) / sizeof(realistic_area_codes[0]))];
    
    // Central office code (prefix): NXX format (first digit 2-9, others 0-9)
    // Avoid 555 prefix (traditionally reserved for fiction) and special codes
    int prefix_first = 2 + random(8);  // 2-9
    int prefix_second, prefix_third;
    
    do {
        prefix_second = random(10);    // 0-9  
        prefix_third = random(10);     // 0-9
        
        int prefix = prefix_first * 100 + prefix_second * 10 + prefix_third;
        if (prefix == 555 || prefix == 911 || prefix == 411 || prefix == 611) {
            continue;  // Avoid reserved prefixes
        }
        break;
    } while (true);
    
    // Subscriber number (suffix): XXXX format (all digits 0-9)
    // Avoid patterns like 0000, 1111, 1234 for more realism
    int suffix_1, suffix_2, suffix_3, suffix_4;
    
    do {
        suffix_1 = random(10);
        suffix_2 = random(10);
        suffix_3 = random(10);
        suffix_4 = random(10);
        
        // Check for obviously fake patterns
        if (suffix_1 == suffix_2 && suffix_2 == suffix_3 && suffix_3 == suffix_4) {
            continue;  // Avoid 0000, 1111, etc.
        }
        if (suffix_1 == 1 && suffix_2 == 2 && suffix_3 == 3 && suffix_4 == 4) {
            continue;  // Avoid 1234
        }
        break;
    } while (true);
    
    // Format as complete 11-digit NANP number: 1NXXNXXXXXX
    snprintf(_generated_number, sizeof(_generated_number), "%c%03d%d%d%d%d%d%d%d",
             country_code,
             area_code,
             prefix_first, prefix_second, prefix_third,
             suffix_1, suffix_2, suffix_3, suffix_4);
}

// Override frequency offset methods to use DTMF frequencies
float SimDTMF::getFrequencyOffsetA() const {
    return _current_row_freq;  // Row frequency
}

float SimDTMF::getFrequencyOffsetC() const {
    return _current_col_freq;  // Column frequency
}

void SimDTMF::debug_print_phone_number() const {
    if (_use_random_numbers) {
        Serial.print("Random DTMF Phone Number: ");
        Serial.println(_generated_number);
    } else {
        Serial.print("Fixed DTMF Sequence: ");
        Serial.println(_digit_sequence);
    }
}
