#include "sim_exchange.h"
#include "signal_meter.h"
#include "Arduino.h"

SimExchange::SimExchange(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, 
                         ExchangeSignalType signal_type)
    : SimTransmitter(wave_gen_pool, fixed_freq),
      _signal_type(signal_type),
      _signal_meter(signal_meter),
      _realizer_b(-1),
      _dual_generator_mode(false),
      _last_state_change(0),
      _tone_active(false),
      _error_tone_step(0),
      _current_freq_a(0.0),
      _current_freq_b(0.0)
{
}

bool SimExchange::acquire_second_generator()
{
    _realizer_b = _wave_gen_pool->get_realizer();
    if(_realizer_b != -1) {
        Serial.print("SimExchange: Acquired second generator #");
        Serial.println(_realizer_b);
        
        // Initialize second generator
        WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
        if(wavegen_b) {
            wavegen_b->set_frequency(SILENT_FREQ, true);  // Start silent
            wavegen_b->set_frequency(SILENT_FREQ, false);
            _dual_generator_mode = true;
            return true;
        }
        Serial.println("SimExchange: ERROR - Could not access second generator");
    }
    Serial.println("SimExchange: WARNING - No second generator available");
    return false;
}

void SimExchange::release_second_generator()
{
    if(_realizer_b != -1) {
        Serial.print("SimExchange: Releasing second generator #");
        Serial.println(_realizer_b);
        
        // Silence the second generator before release
        WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
        if(wavegen_b) {
            wavegen_b->set_frequency(SILENT_FREQ, true);
            wavegen_b->set_frequency(SILENT_FREQ, false);
        }
        
        // Release the second generator
        _wave_gen_pool->free_realizer(_realizer_b);
        _realizer_b = -1;
        _dual_generator_mode = false;
    }
}

bool SimExchange::begin(unsigned long time)
{
    Serial.print("SimExchange: Starting telephony signal: ");
    switch(_signal_type) {
        case EXCHANGE_DIAL_TONE: Serial.println("DIAL TONE"); break;
        case EXCHANGE_RINGING: Serial.println("RINGING"); break;
        case EXCHANGE_BUSY: Serial.println("BUSY"); break;
        case EXCHANGE_REORDER: Serial.println("REORDER"); break;
        case EXCHANGE_ERROR: Serial.println("ERROR TONE"); break;
        case EXCHANGE_SILENT: Serial.println("SILENT"); break;
        default: Serial.println("UNKNOWN"); break;
    }
    
    // Initialize base transmitter first
    if(!Realization::begin(time)) {
        Serial.println("SimExchange: ERROR - Could not initialize base realizer");
        return false;
    }
    
    // Print primary generator assignment for debugging
    if(_realizer != -1) {
        Serial.print("SimExchange: Primary generator assigned: #");
        Serial.println(_realizer);
    }
    
    // Attempt to acquire second generator for dual-tone operation
    acquire_second_generator();
    
    // Reset state machine
    _last_state_change = time;
    _tone_active = false;
    _error_tone_step = 0;
    
    return true;
}

void SimExchange::end()
{
    // Release second generator first
    release_second_generator();
    
    // Release primary generator using inherited method
    SimTransmitter::end();
}

bool SimExchange::update(Mode *mode)
{
    // Call inherited update for basic frequency management
    common_frequency_update(mode);
    
    // Check if frequency is within audible bounds
    if (!check_frequency_bounds()) {
        return true;  // Continue running but station will be silenced
    }
    
    // The actual wave generator frequency updates are handled by realize() -> set_dual_tone()
    // This ensures consistent VFO tracking for all signal types and timing states
    
    return true;
}

bool SimExchange::step(unsigned long time)
{
    // Update timing and realize telephony signals
    realize();
    return true;  // Always continue running
}

void SimExchange::randomize()
{
    // Randomize telephony signal type for realistic exchange behavior
    // Weight towards common signals: dial tone and busy are more common than error tones
    
    int signal_choice = random(100);  // 0-99
    
    if(signal_choice < 30) {
        // 30% chance: Dial tone (most common)
        _signal_type = EXCHANGE_DIAL_TONE;
        Serial.println("SimExchange: Randomized to DIAL_TONE");
    }
    else if(signal_choice < 50) {
        // 20% chance: Busy signal (common)
        _signal_type = EXCHANGE_BUSY;
        Serial.println("SimExchange: Randomized to BUSY_SIGNAL");
    }
    else if(signal_choice < 70) {
        // 20% chance: Ringing (common)
        _signal_type = EXCHANGE_RINGING;
        Serial.println("SimExchange: Randomized to RINGING");
    }
    else if(signal_choice < 85) {
        // 15% chance: Reorder (less common)
        _signal_type = EXCHANGE_REORDER;
        Serial.println("SimExchange: Randomized to REORDER");
    }
    else if(signal_choice < 95) {
        // 10% chance: Error tone (uncommon)
        _signal_type = EXCHANGE_ERROR;
        Serial.println("SimExchange: Randomized to ERROR_TONE");
    }
    else {
        // 5% chance: Silent (rare, for maintenance periods)
        _signal_type = EXCHANGE_SILENT;
        Serial.println("SimExchange: Randomized to SILENT");
    }
    
    // Reset state machine for new signal type
    _last_state_change = millis();
    _tone_active = false;
    _error_tone_step = 0;
    
    // If we have an active realizer, immediately start the new signal
    if(_realizer != -1) {
        realize();
    }
}

void SimExchange::debug_print_signal_info() const
{
    Serial.println("=== SimExchange Signal Debug Info ===");
    Serial.print("Signal Type: ");
    switch(_signal_type) {
        case EXCHANGE_DIAL_TONE: Serial.print("DIAL_TONE"); break;
        case EXCHANGE_RINGING: Serial.print("RINGING"); break;
        case EXCHANGE_BUSY: Serial.print("BUSY"); break;
        case EXCHANGE_REORDER: Serial.print("REORDER"); break;
        case EXCHANGE_ERROR: Serial.print("ERROR_TONE"); break;
        case EXCHANGE_SILENT: Serial.print("SILENT"); break;
        default: Serial.print("UNKNOWN"); break;
    }
    Serial.println();
    
    Serial.print("Dual Generator Mode: ");
    Serial.println(_dual_generator_mode ? "YES" : "NO");
    
    Serial.print("Primary Generator (#");
    Serial.print(_realizer);
    Serial.print("): ");
    Serial.print(_current_freq_a, 1);
    Serial.println(" Hz");
    
    if(_dual_generator_mode) {
        Serial.print("Secondary Generator (#");
        Serial.print(_realizer_b);
        Serial.print("): ");
        Serial.print(_current_freq_b, 1);
        Serial.println(" Hz");
    }
    
    Serial.print("Tone Active: ");
    Serial.println(_tone_active ? "YES" : "NO");
    
    Serial.print("Time Since Last State Change: ");
    Serial.print(millis() - _last_state_change);
    Serial.println(" ms");
    
    if(_signal_type == EXCHANGE_ERROR) {
        Serial.print("Error Tone Step: ");
        Serial.println(_error_tone_step);
    }
    
    Serial.println("=======================================");
}

void SimExchange::realize()
{
    unsigned long current_time = millis();
    
    // Ensure we have a primary generator
    if(_realizer == -1) {
        return;
    }
    
    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    if(!wavegen) return;
    
    switch(_signal_type) {
        case EXCHANGE_DIAL_TONE:
            realize_dial_tone(current_time, wavegen);
            break;
        case EXCHANGE_RINGING:
            realize_ringing(current_time, wavegen);
            break;
        case EXCHANGE_BUSY:
            realize_busy(current_time, wavegen);
            break;
        case EXCHANGE_REORDER:
            realize_reorder(current_time, wavegen);
            break;
        case EXCHANGE_ERROR:
            realize_error_tone(current_time, wavegen);
            break;
        case EXCHANGE_SILENT:
            // Silent mode - set all generators to silent frequency
            set_dual_tone(SILENT_FREQ, SILENT_FREQ);
            _tone_active = false;
            break;
    }
    
    // Add signal strength to meter for proximity indication
    if(_tone_active && _signal_meter) {
        _signal_meter->add_charge(_frequency);
    }
}

void SimExchange::realize_dial_tone(unsigned long current_time, WaveGen *wavegen)
{
    // Dial tone is continuous dual-tone (350 + 440 Hz)
    set_dual_tone(EXCHANGE_DIAL_TONE_LOW, EXCHANGE_DIAL_TONE_HIGH);
    _tone_active = true;
}

void SimExchange::realize_ringing(unsigned long current_time, WaveGen *wavegen)
{
    // Ringing: 2 seconds on, 4 seconds off (440 + 480 Hz)
    unsigned long cycle_time = current_time - _last_state_change;
    
    if(!_tone_active && cycle_time >= EXCHANGE_RINGING_OFF_TIME) {
        // Start ringing
        set_dual_tone(EXCHANGE_RINGING_LOW, EXCHANGE_RINGING_HIGH);
        _tone_active = true;
        _last_state_change = current_time;
    }
    else if(_tone_active && cycle_time >= EXCHANGE_RINGING_ON_TIME) {
        // Stop ringing  
        set_dual_tone(SILENT_FREQ, SILENT_FREQ);
        _tone_active = false;
        _last_state_change = current_time;
    }
}

void SimExchange::realize_busy(unsigned long current_time, WaveGen *wavegen)
{
    // Busy signal: 500ms on, 500ms off (480 + 620 Hz)
    unsigned long cycle_time = current_time - _last_state_change;
    
    if(!_tone_active && cycle_time >= EXCHANGE_BUSY_OFF_TIME) {
        // Start busy tone
        set_dual_tone(EXCHANGE_BUSY_LOW, EXCHANGE_BUSY_HIGH);
        _tone_active = true;
        _last_state_change = current_time;
    }
    else if(_tone_active && cycle_time >= EXCHANGE_BUSY_ON_TIME) {
        // Stop busy tone
        set_dual_tone(SILENT_FREQ, SILENT_FREQ);
        _tone_active = false;
        _last_state_change = current_time;
    }
}

void SimExchange::realize_reorder(unsigned long current_time, WaveGen *wavegen)
{
    // Reorder signal: 250ms on, 250ms off (480 + 620 Hz) - faster than busy
    unsigned long cycle_time = current_time - _last_state_change;
    
    if(!_tone_active && cycle_time >= EXCHANGE_REORDER_OFF_TIME) {
        // Start reorder tone
        set_dual_tone(EXCHANGE_REORDER_LOW, EXCHANGE_REORDER_HIGH);
        _tone_active = true;
        _last_state_change = current_time;
    }
    else if(_tone_active && cycle_time >= EXCHANGE_REORDER_ON_TIME) {
        // Stop reorder tone
        set_dual_tone(SILENT_FREQ, SILENT_FREQ);
        _tone_active = false;
        _last_state_change = current_time;
    }
}

void SimExchange::realize_error_tone(unsigned long current_time, WaveGen *wavegen)
{
    // Error tone: 3-tone sequence with specific timing from header
    unsigned long cycle_time = current_time - _last_state_change;
    
    // Total cycle time: tone1(380) + gap(30) + tone2(276) + gap(30) + tone3(380) + silence(2000) = 3096ms
    unsigned long total_cycle = EXCHANGE_ERROR_TONE1_TIME + 30 + EXCHANGE_ERROR_TONE2_TIME + 30 + EXCHANGE_ERROR_TONE3_TIME + EXCHANGE_ERROR_SILENCE_TIME;
    
    if(cycle_time >= total_cycle) {
        // Reset cycle
        _error_tone_step = 0;
        _last_state_change = current_time;
        cycle_time = 0;
    }
    
    // Determine current step in the sequence
    if(cycle_time < EXCHANGE_ERROR_TONE1_TIME) {
        // First tone (913.8 Hz)
        if(_error_tone_step != 0) {
            set_single_tone(EXCHANGE_ERROR_TONE1);
            _error_tone_step = 0;
            _tone_active = true;
        }
    }
    else if(cycle_time < EXCHANGE_ERROR_TONE1_TIME + 30) {
        // First gap
        if(_tone_active) {
            set_single_tone(SILENT_FREQ);
            _tone_active = false;
        }
    }
    else if(cycle_time < EXCHANGE_ERROR_TONE1_TIME + 30 + EXCHANGE_ERROR_TONE2_TIME) {
        // Second tone (1428.5 Hz)
        if(_error_tone_step != 1) {
            set_single_tone(EXCHANGE_ERROR_TONE2);
            _error_tone_step = 1;
            _tone_active = true;
        }
    }
    else if(cycle_time < EXCHANGE_ERROR_TONE1_TIME + 30 + EXCHANGE_ERROR_TONE2_TIME + 30) {
        // Second gap
        if(_tone_active) {
            set_single_tone(SILENT_FREQ);
            _tone_active = false;
        }
    }
    else if(cycle_time < EXCHANGE_ERROR_TONE1_TIME + 30 + EXCHANGE_ERROR_TONE2_TIME + 30 + EXCHANGE_ERROR_TONE3_TIME) {
        // Third tone (1776.7 Hz)
        if(_error_tone_step != 2) {
            set_single_tone(EXCHANGE_ERROR_TONE3);
            _error_tone_step = 2;
            _tone_active = true;
        }
    }
    else {
        // Final silence before repeat
        if(_tone_active) {
            set_single_tone(SILENT_FREQ);
            _tone_active = false;
        }
    }
}

void SimExchange::set_single_tone(float frequency)
{
    // Set primary generator to frequency, secondary silent
    if(_realizer != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        if(wavegen) {
            wavegen->set_frequency(frequency, true);
            wavegen->set_frequency(frequency, false);
            _current_freq_a = frequency;
        }
    }
    
    // Silence secondary generator if available
    if(_dual_generator_mode && _realizer_b != -1) {
        WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
        if(wavegen_b) {
            wavegen_b->set_frequency(SILENT_FREQ, true);
            wavegen_b->set_frequency(SILENT_FREQ, false);
            _current_freq_b = SILENT_FREQ;
        }
    }
}

void SimExchange::set_dual_tone(float freq_a, float freq_b)
{
    // Set primary generator to freq_a
    if(_realizer != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        if(wavegen) {
            wavegen->set_frequency(freq_a, true);
            wavegen->set_frequency(freq_a, false);
            _current_freq_a = freq_a;
        }
    }
    
    // Set secondary generator to freq_b if available
    if(_dual_generator_mode && _realizer_b != -1) {
        WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
        if(wavegen_b) {
            wavegen_b->set_frequency(freq_b, true);
            wavegen_b->set_frequency(freq_b, false);
            _current_freq_b = freq_b;
        }
    } else {
        // Fallback: Approximate dual-tone with frequency modulation on primary generator
        // This isn't perfect but provides some telephony character when only one generator is available
        float mixed_freq = (freq_a + freq_b) / 2.0;  // Simple frequency averaging
        if(_realizer != -1) {
            WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
            if(wavegen) {
                wavegen->set_frequency(mixed_freq, true);
                wavegen->set_frequency(mixed_freq, false);
                _current_freq_a = mixed_freq;
                _current_freq_b = 0.0;  // Indicate no true dual-tone
            }
        }
    }
}
