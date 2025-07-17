// Field Day Configuration - Must be defined before including sim_station.h
#include "station_config.h"

// Temporary debug output for resource testing - disable for production to save memory
// #define DEBUG_STATION_RESOURCES

#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_telco2.h"
#include "signal_meter.h"

#define WAIT_SECONDS2 4

// Telephony frequency offset constants (authentic DTMF frequencies)
const float SimTelco2::RINGBACK_FREQ_A = 440.0f;  // 440 Hz for ringback tone
const float SimTelco2::RINGBACK_FREQ_C = 480.0f;  // 480 Hz for ringback tone  
const float SimTelco2::BUSY_FREQ_A = 480.0f;      // 480 Hz for busy/reorder signals
const float SimTelco2::BUSY_FREQ_C = 620.0f;      // 620 Hz for busy/reorder signals
const float SimTelco2::DIAL_FREQ_A = 350.0f;      // 350 Hz for dial tone
const float SimTelco2::DIAL_FREQ_C = 440.0f;      // 440 Hz for dial tone

// Legacy aliases for backward compatibility
const float SimTelco2::RING_FREQ_A = SimTelco2::RINGBACK_FREQ_A;
const float SimTelco2::RING_FREQ_C = SimTelco2::RINGBACK_FREQ_C;

// DTMF frequency lookup tables (based on reference call_sequence.ino)
const float SimTelco2::ROW_FREQUENCIES[4] = {
    DTMF_ROW_1,  // 697.0 Hz
    DTMF_ROW_2,  // 770.0 Hz  
    DTMF_ROW_3,  // 852.0 Hz
    DTMF_ROW_4   // 941.0 Hz
};

const float SimTelco2::COL_FREQUENCIES[4] = {
    DTMF_COL_1,  // 1209.0 Hz
    DTMF_COL_2,  // 1336.0 Hz
    DTMF_COL_3,  // 1477.0 Hz
    DTMF_COL_4   // 1633.0 Hz
};

// DTMF digit to row mapping (from reference call_sequence.ino)
const int SimTelco2::DIGIT_TO_ROW[16] = {
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
const int SimTelco2::DIGIT_TO_COL[16] = {
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

// mode is expected to be a derivative of VFO
SimTelco2::SimTelco2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, TelcoType type)
    : SimDualTone(wave_gen_pool, fixed_freq), _signal_meter(signal_meter), _telco_type(type)
{
    // Set frequency offsets based on telco type
    // setFrequencyOffsetsForType();
    
    // Configure AsyncTelco timing based on telco type
    _telco.configure_timing(type);
    
    // Initialize operator frustration drift tracking
    _cycles_completed = 0;
    _cycles_until_qsy = 30 + (random(30));   // 3-8 cycles before frustration (realistic)

    // Initialize timing state
    _in_wait_delay = false;
    _next_cycle_time = 0;

    // from SimDTMF
    // _current_row_freq = 0.0f;
    // _current_col_freq = 0.0f;
    _use_random_numbers = true;
    
    // Generate initial random phone number
    generate_random_nanp_number();
    _digit_sequence = _generated_number;  // Point to generated number

    // // Initialize wait delay management (matching SimTelco pattern)
    // // _in_wait_delay = false;
    // // _next_cycle_time = 0;
}

bool SimTelco2::begin(unsigned long time){
    // Attempt to acquire all required realizers atomically
    // The new Realization::begin() handles dual generator coordination automatically
    if(!common_begin(time, _fixed_freq)) {
        return false;  // Failed to acquire all needed wave generators
    }

    // Initialize all acquired wave generators
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

    // Set enabled and force frequency update with existing _vfo_freq
    // _vfo_freq should retain its value from the previous cycle
    _enabled = true;

    // this call is ineffective because _active is false
    // force_frequency_update();

    realize();  // CRITICAL: Set active state for audio output!

    // Start AsyncTelco ring cadence (repeating)
    _telco.start_telco_transmission(true);
    _in_wait_delay = false;

    // from SimDTMF
    // Start AsyncDTMF sequence (matching SimTelco pattern with AsyncTelco)
    _dtmf.start_dtmf_transmission(_digit_sequence, true);
    // _in_wait_delay = false;  // Clear wait delay state

    return true;
}

void SimTelco2::realize(){
    if(!has_all_realizers()) {
        Serial.println("*******");
        Serial.println("no wave gens allocated");
        return;
    }
    
    if(!check_frequency_bounds()) {
        Serial.println("&&&&&&&");
        Serial.println("out of bounds");
        return;
    }

    // Serial.println("realize*******");
    
    // Set active state for all acquired wave generators
    int realizer_index = 0;

    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);

        // wavegen_a->set_frequency(_frequency);
        // wavegen_a->set_frequency(SILENT_FREQ, false);
        // Serial.println("would set:");
        // Serial.println(_frequency);

        wavegen_a->set_active_frequency(_active);
    }

    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);

        // wavegen_c->set_frequency(_frequency2);
        // wavegen_c->set_frequency(SILENT_FREQ, false);
        // Serial.println("would set:");
        // Serial.println(_frequency2);

        wavegen_c->set_active_frequency(_active);
    }
}

// returns true on successful update
bool SimTelco2::update(Mode *mode){
    common_frequency_update(mode);

    if(!_enabled){
        Serial.println("update() not enabled");
    }

    if(!has_all_realizers()){
        Serial.println("not all realizers");
    }

    if(_enabled && has_all_realizers()){

        Serial.print("+++Update");

        // Update frequencies for all acquired wave generators
        int realizer_index = 0;

        int realizer_a = get_realizer(realizer_index++);
        if(realizer_a != -1){
            WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
            Serial.print("update ");
            Serial.print(_frequency);
            Serial.print(SILENT_FREQ);
            wavegen_a->set_frequency(_frequency);
            wavegen_a->set_frequency(SILENT_FREQ, false);
        }

        int realizer_c = get_realizer(realizer_index++);
        if(realizer_c != -1){
            WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
            Serial.print("update ");
            Serial.print(_frequency2);
            Serial.print(SILENT_FREQ);
            wavegen_c->set_frequency(_frequency2);
            wavegen_c->set_frequency(SILENT_FREQ, false);
        }
    }

    realize();
    return true;
}

// call periodically to keep realization dynamic
// returns true if it should keep going
bool SimTelco2::step(unsigned long time){
    // // Handle ring cadence timing using AsyncTelco
    // int telco_state = _telco.step_telco(time);
    
    // switch(telco_state) {
    //     case STEP_TELCO_TURN_ON:
    //         _active = true;
    //         realize();
    //         send_carrier_charge_pulse(_signal_meter);  // Send charge pulse when carrier turns on
    //         break;
            
    //     case STEP_TELCO_LEAVE_ON:
    //         // Carrier remains on - send another charge pulse
    //         send_carrier_charge_pulse(_signal_meter);
    //         break;
            
    //     case STEP_TELCO_TURN_OFF:
    //         _active = false;
    //         realize();
    //         // No charge pulse when carrier turns off
            
    //         // Count completed cycles for frustration logic (when ring cycle ends)
    //         _cycles_completed++;
    //         if(_cycles_completed >= _cycles_until_qsy) {
    //             // Operator gets frustrated, QSYs to new frequency
    //             apply_operator_frustration_drift();
    //             // Reset frustration counter for next QSY
    //             _cycles_completed = 0;
    //             _cycles_until_qsy = 30 + (random(30));   // 3-8 cycles before next frustration
    //         }
    //         break;
            
    //     case STEP_TELCO_LEAVE_OFF:
    //         // Carrier remains off - no action needed
    //         break;
            
    //     case STEP_TELCO_CHANGE_FREQ:
    //         // Continue transmitting but change frequency (for dual-tone systems)
    //         // Ring uses single tone, but this prepares for other telephony sounds
    //         send_carrier_charge_pulse(_signal_meter);
    //         break;
    // }

    // from SimDTMF
    // Handle DTMF sequence timing using AsyncDTMF (matching SimTelco pattern)
    int dtmf_state = _dtmf.step_dtmf(time);
    
    switch(dtmf_state) {
        case STEP_DTMF_TURN_ON:
            // from telco state
            // _active = true;
            // realize();
            // send_carrier_charge_pulse(_signal_meter);  // Send charge pulse when carrier turns on
            // break;


        // New digit starting - set frequencies and activate
            _active = true;
            set_digit_frequencies(_dtmf.get_current_digit());
            force_frequency_update2(); //NOT IN TELCO
            // update();

            realize();
            send_carrier_charge_pulse(_signal_meter);
            break;
            
        case STEP_DTMF_LEAVE_ON:
            // SAME IN TELCO

            // Carrier remains on - send another charge pulse
            send_carrier_charge_pulse(_signal_meter);
            break;
            
        case STEP_DTMF_TURN_OFF:
            // from telco
            // Serial.println("+++TURN OFF");
            // force_frequency_update(); //NOT IN TELCO
            _active = false;
            realize();
            // No charge pulse when carrier turns off
            
            // NOT IN TELCO
            // // Count completed cycles for frustration logic (when ring cycle ends)
            // _cycles_completed++;
            // if(_cycles_completed >= _cycles_until_qsy) {
            //     // Operator gets frustrated, QSYs to new frequency
            //     apply_operator_frustration_drift();
            //     // Reset frustration counter for next QSY
            //     _cycles_completed = 0;
            //     _cycles_until_qsy = 30 + (random(30));   // 3-8 cycles before next frustration
            // }
            // break;

            // Enter silence - set silent frequencies but keep generators
            // _current_row_freq = 0.0f; // NOT IN TELCO
            // _current_col_freq = 0.0f; // NOT IN TELCO
            // force_frequency_update(); // NOT IN TELCO
            // Note: Keep _active = true to maintain generator allocation
            break;
            
        case STEP_DTMF_LEAVE_OFF:
            // SAME IN TELCO

            // Remain in silence - no action needed
            break;
            
        case STEP_DTMF_CYCLE_END:
            // End of sequence - properly release wave generators using end()
            end();  // This calls SimDualTone::end() -> Realization::end() to free realizers
            _in_wait_delay = true;
            _next_cycle_time = time + 3000;  // 3 second pause
            break;
    }
    
// // // // Check if it's time to restart sequence after wait delay (matching SimTelco pattern)
// // // if(_in_wait_delay && time >= _next_cycle_time) {
// // //     // DYNAMIC PIPELINING: Try to reallocate WaveGen for next sequence cycle
// // //     if(begin(time)) {  // Only proceed if WaveGen is available
// // //         _in_wait_delay = false;
// // //     } else {
// // //         // WaveGen not available - extend wait period and try again later
// // //         _next_cycle_time = time + 500 + random(1000);     // Try again in 0.5-1.5 seconds
// // //     }
// // // }

    // Check if it's time to start next transmission cycle after a wait period
    if(_in_wait_delay && time >= _next_cycle_time) {
        // DYNAMIC PIPELINING: Try to reallocate WaveGen for next transmission cycle
        if(begin(time)) {  // Only proceed if WaveGen is available
            _in_wait_delay = false;
        } else {
            // WaveGen not available - extend wait period and try again later
            // Add randomization to prevent thundering herd problem
            _next_cycle_time = time + 500 + random(1000);     // Try again in 0.5-1.5 seconds
        }
    }

    return true;
}

void SimTelco2::set_digit_frequencies(char digit) {
    // Serial.println("----------");
    // Serial.println(digit);
    int digit_index = char_to_digit_index(digit);
    
    if(digit_index >= 0 && digit_index < 16) {
        int row_index = DIGIT_TO_ROW[digit_index];
        int col_index = DIGIT_TO_COL[digit_index];
        
        // _current_row_freq = ROW_FREQUENCIES[row_index];
        // _current_col_freq = COL_FREQUENCIES[col_index];
        _frequency_offset_a = ROW_FREQUENCIES[row_index];
        _frequency_offset_c = COL_FREQUENCIES[col_index];
    } else {
        // Invalid character - use silence
        // _current_row_freq = 0.0f;
        // _current_col_freq = 0.0f;
        _frequency_offset_a = SILENT_FREQ;
        _frequency_offset_c = SILENT_FREQ;
    }
    // Serial.println(_frequency_offset_a);
    // Serial.println(_frequency_offset_c);
}

int SimTelco2::char_to_digit_index(char c) {
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

void SimTelco2::generate_random_nanp_number() {
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

// // Override frequency offset methods to use DTMF frequencies
// float SimTelco2::getFrequencyOffsetA() const {
//     return _current_row_freq;  // Row frequency
// }

// float SimTelco2::getFrequencyOffsetC() const {
//     return _current_col_freq;  // Column frequency
// }

// Set station into retry state (used when initialization fails)
void SimTelco2::set_retry_state(unsigned long next_try_time) {
    _in_wait_delay = true;
    _next_cycle_time = next_try_time;
}

void SimTelco2::apply_operator_frustration_drift()
{
    // Move to a new frequency as if a whole new operator is on the air
    // Realistic amateur radio operator frequency adjustment
    // ±250 Hz - keep nearby within listening range
    const float DRIFT_RANGE = 250.0f;

    float drift = ((float)random(0, (long)(2.0f * DRIFT_RANGE * 100))) / 100.0f - DRIFT_RANGE;

    // Apply drift to the shared frequency
    _fixed_freq = _fixed_freq + drift;

    // Immediately update the wave generator frequency
    force_frequency_update();
}

// left out because it could be causing the bug
// void SimTelco2::randomize() {
//     // CRITICAL: Ensure proper cleanup when station gets moved by StationManager
//     // This prevents wave generators from getting "stuck on" during station moves
//     end();  // Release all wave generators before randomizing
    
//     // Generate new random phone number if using random generation
//     if (_use_random_numbers) {
//         generate_random_nanp_number();
//         _digit_sequence = _generated_number;  // Update pointer to new number
//         Serial.print("DTMF: Generated new random number: ");
//         Serial.println(_generated_number);
//     }
    
//     // Reset AsyncDTMF sequence
//     _dtmf.reset_sequence();
    
//     // Reset wait delay state
//     _in_wait_delay = false;
//     _next_cycle_time = 0;
// }

void SimTelco2::randomize()
{
    // Re-randomize station properties for realistic relocation behavior
    
    // Reset cycle counters to make station behavior feel fresh
    _cycles_completed = 0;
    
    // Set a new random frustration threshold (cycles until QSY)
    _cycles_until_qsy = random(3, 11);  // 3-10 cycles before getting frustrated
    
    // Reset timing state
    _in_wait_delay = false;
    _next_cycle_time = 0;  // Will be set properly on next cycle
}

// Set frequency offsets based on telco type
void SimTelco2::setFrequencyOffsetsForType() {
    switch (_telco_type) {
        case TELCO_RINGBACK:
            _frequency_offset_a = RINGBACK_FREQ_A;  // 440 Hz
            _frequency_offset_c = RINGBACK_FREQ_C;  // 480 Hz
            break;
            
        case TELCO_BUSY:
        case TELCO_REORDER:
            _frequency_offset_a = BUSY_FREQ_A;  // 480 Hz
            _frequency_offset_c = BUSY_FREQ_C;  // 620 Hz
            break;
            
        case TELCO_DIALTONE:
            _frequency_offset_a = DIAL_FREQ_A;  // 350 Hz
            _frequency_offset_c = DIAL_FREQ_C;  // 440 Hz
            break;
            
        default:
            // Default to ringback frequencies as fallback
            _frequency_offset_a = RINGBACK_FREQ_A;
            _frequency_offset_c = RINGBACK_FREQ_C;
            break;
    }
}

// Override frequency offset methods to use stored values instead of macros
float SimTelco2::getFrequencyOffsetA() const {
    return _frequency_offset_a;
}

float SimTelco2::getFrequencyOffsetC() const {
    return _frequency_offset_c;
}
