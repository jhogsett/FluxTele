#include "sim_dualtone.h"
#include "wavegen.h"
#include "vfo.h"
#include "saved_data.h"  // For option_bfo_offset

SimDualTone::SimDualTone(WaveGenPool *wave_gen_pool, float fixed_freq, 
                         float tone_offset_a, float tone_offset_b) 
    : Realization(wave_gen_pool, (int)(fixed_freq / 1000), 
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
        2  // Dual generator mode requires 2 realizers
#elif defined(ENABLE_GENERATOR_A) || defined(ENABLE_GENERATOR_C)
        1  // Single generator mode requires 1 realizer
#else
        1  // Default fallback
#endif
    )
{
    // Initialize shared station properties
    _fixed_freq = fixed_freq;
    _enabled = false;
    _active = false;
    _vfo_freq = 0.0;  // Initialize VFO frequency to prevent garbage values

    // Initialize telephony tone offsets
    _tone_offset_a = tone_offset_a;
    _tone_offset_b = tone_offset_b;

#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Initialize Wave Generator A variables (primary tone)
    _frequency = 0.0;
    
    // Initialize Wave Generator C variables (secondary tone)
    _frequency_c = 0.0;
    
    // Initialize dynamic station management state (shared)
    _station_state = DORMANT_DT;
#elif defined(ENABLE_GENERATOR_A)
    // Initialize Wave Generator A variables (single tone mode)
    _frequency = 0.0;
    
    // Initialize dynamic station management state
    _station_state = DORMANT_DT;
#elif defined(ENABLE_GENERATOR_C)
    // Initialize Wave Generator C variables (single tone mode)
    _frequency_c = 0.0;
    
    // Initialize dynamic station management state
    _station_state_c = DORMANT_DT;
#endif
}

bool SimDualTone::common_begin(unsigned long time, float fixed_freq)
{
    // Set shared properties first
    _fixed_freq = fixed_freq;
    
    // Update station ID for debugging (frequency in kHz)
    set_station_id((int)(fixed_freq / 1000));
    
    // Attempt to acquire all required realizers atomically
    bool success = Realization::begin(time);
    if(!success) {
        return false;  // Failed to acquire all needed realizers
    }
    
    // Initialize generator-specific variables
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    _frequency = 0.0;
    _frequency_c = 0.0;
#elif defined(ENABLE_GENERATOR_A)
    _frequency = 0.0;
#elif defined(ENABLE_GENERATOR_C)
    _frequency_c = 0.0;
#endif
    
    return true;
}

void SimDualTone::common_frequency_update(Mode *mode)
{
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Note: mode is expected to be a VFO object
    VFO *vfo = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo->_frequency) + (vfo->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency = _vfo_freq - _fixed_freq;
    
    // Add BFO offset and telephony tone offsets for authentic dual-tone telephony
    _frequency = raw_frequency + option_bfo_offset + _tone_offset_a;
    _frequency_c = raw_frequency + option_bfo_offset + _tone_offset_b;
#elif defined(ENABLE_GENERATOR_A)
    // Note: mode is expected to be a VFO object
    VFO *vfo = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo->_frequency) + (vfo->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency = _vfo_freq - _fixed_freq;
    
    // Add BFO offset and first tone offset for single-tone mode
    _frequency = raw_frequency + option_bfo_offset + _tone_offset_a;
#elif defined(ENABLE_GENERATOR_C)
    // Note: mode is expected to be a VFO object
    VFO *vfo_c = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo_c->_frequency) + (vfo_c->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency_c = _vfo_freq - _fixed_freq;
    
    // Add BFO offset and first tone offset for single-tone mode
    _frequency_c = raw_frequency_c + option_bfo_offset + _tone_offset_a;
#endif
}

bool SimDualTone::check_frequency_bounds()
{
    // Check frequency bounds for all enabled generators
    bool any_in_bounds = false;

#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Check primary tone bounds
    if(_frequency > MAX_AUDIBLE_FREQ_DT || _frequency < MIN_AUDIBLE_FREQ_DT){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ_DT, true);
                    wavegen->set_frequency(SILENT_FREQ_DT, false);
                }
            }
        }
    } else {
        any_in_bounds = true;
    }

    // Check secondary tone bounds
    if(_frequency_c > MAX_AUDIBLE_FREQ_DT || _frequency_c < MIN_AUDIBLE_FREQ_DT){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ_DT);
                }
            }
        }
    } else {
        any_in_bounds = true;
    }
#elif defined(ENABLE_GENERATOR_A)
    if(_frequency > MAX_AUDIBLE_FREQ_DT || _frequency < MIN_AUDIBLE_FREQ_DT){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ_DT, true);
                    wavegen->set_frequency(SILENT_FREQ_DT, false);
                }
            }
        }
    } else {
        any_in_bounds = true;
    }
#elif defined(ENABLE_GENERATOR_C)
    if(_frequency_c > MAX_AUDIBLE_FREQ_DT || _frequency_c < MIN_AUDIBLE_FREQ_DT){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ_DT);
                }
            }
        }
    } else {
        any_in_bounds = true;
    }
#endif

    if(any_in_bounds && !_enabled){
        _enabled = true;
    }
    
    return any_in_bounds;
}

void SimDualTone::end()
{
    _enabled = false;
    _active = false;
    
    // Release all acquired realizers
    Realization::end();
}

void SimDualTone::force_wave_generator_refresh()
{
    // Force all wave generators to update their frequencies
    force_frequency_update();
}

bool SimDualTone::reinitialize(unsigned long time, float fixed_freq)
{
    // End current session safely
    end();
    
    // Attempt to begin with new frequency
    return common_begin(time, fixed_freq);
}

void SimDualTone::randomize()
{
    // Default implementation does nothing
    // Derived classes can override for station-specific randomization
}

void SimDualTone::set_station_state(DualToneState new_state)
{
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    _station_state = new_state;
#elif defined(ENABLE_GENERATOR_A)
    _station_state = new_state;
#elif defined(ENABLE_GENERATOR_C)
    _station_state_c = new_state;
#endif
}

DualToneState SimDualTone::get_station_state() const
{
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    return _station_state;
#elif defined(ENABLE_GENERATOR_A)
    return _station_state;
#elif defined(ENABLE_GENERATOR_C)
    return _station_state_c;
#else
    return DORMANT_DT;  // Fallback
#endif
}

bool SimDualTone::is_audible() const
{
    return _enabled && (get_realizer_count() > 0);
}

float SimDualTone::get_fixed_frequency() const
{
    return _fixed_freq;
}

void SimDualTone::setActive(bool active)
{
    _active = active;
}

bool SimDualTone::isActive() const
{
    return _active;
}

void SimDualTone::set_tone_offsets(float tone_offset_a, float tone_offset_b)
{
    _tone_offset_a = tone_offset_a;
    _tone_offset_b = tone_offset_b;
}

void SimDualTone::get_tone_offsets(float &tone_offset_a, float &tone_offset_b) const
{
    tone_offset_a = _tone_offset_a;
    tone_offset_b = _tone_offset_b;
}

void SimDualTone::force_frequency_update()
{
    if(!_enabled) return;
    
    // Update frequencies on all acquired wave generators
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Dual generator mode: first generator gets primary tone, second gets secondary tone
    if(get_realizer_count() >= 2) {
        int realizer_a = get_realizer(0);
        int realizer_c = get_realizer(1);
        
        if(realizer_a != -1) {
            WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
            wavegen_a->set_frequency(_frequency);
        }
        
        if(realizer_c != -1) {
            WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
            wavegen_c->set_frequency(_frequency_c);
        }
    }
#elif defined(ENABLE_GENERATOR_A)
    // Single generator mode: use primary tone only
    if(get_realizer_count() >= 1) {
        int realizer = get_realizer(0);
        if(realizer != -1) {
            WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
            wavegen->set_frequency(_frequency);
        }
    }
#elif defined(ENABLE_GENERATOR_C)
    // Single generator mode: use primary tone only
    if(get_realizer_count() >= 1) {
        int realizer = get_realizer(0);
        if(realizer != -1) {
            WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
            wavegen->set_frequency(_frequency_c);
        }
    }
#endif
}

void SimDualTone::send_carrier_charge_pulse(SignalMeter* signal_meter)
{
    if(!signal_meter || !_enabled || !_active) return;
    
    // Use VFO charge calculation for signal meter (no BFO offset)
    int charge = VFO::calculate_signal_charge(_fixed_freq, _vfo_freq);
    if(charge > 0) {
        const float LOCK_WINDOW_HZ = 50.0; // Lock window threshold
        float freq_diff = abs(_fixed_freq - _vfo_freq);
        if(freq_diff <= LOCK_WINDOW_HZ) {
            signal_meter->add_charge(-charge);  // Negative for lock indication
        } else {
            signal_meter->add_charge(charge);   // Positive for normal signal
        }
    }
}
