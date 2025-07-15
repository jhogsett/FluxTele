#include "sim_transmitter2.h"
#include "wavegen.h"
#include "vfo.h"
#include "saved_data.h"  // For option_bfo_offset

SimTransmitter2::SimTransmitter2(WaveGenPool *wave_gen_pool, float fixed_freq) 
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

#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Initialize Wave Generator A variables
    _frequency = 0.0;
    _vfo_freq = 0.0;  // Initialize VFO frequency to prevent garbage values
    
    // Initialize dynamic station management state
    _station_state = DORMANT2;

    // Initialize Wave Generator C variables
    _frequency_c = 0.0;
    
    // // Initialize dynamic station management state
    // _station_state_c = DORMANT2;
#elif defined(ENABLE_GENERATOR_A)
    // Initialize Wave Generator A variables
    _frequency = 0.0;
    _vfo_freq = 0.0;  // Initialize VFO frequency to prevent garbage values
    
    // Initialize dynamic station management state
    _station_state = DORMANT2;
#elif defined(ENABLE_GENERATOR_C)
    // Initialize Wave Generator C variables
    _frequency_c = 0.0;
    
    // Initialize dynamic station management state
    _station_state_c = DORMANT2;
#endif
}

bool SimTransmitter2::common_begin(unsigned long time, float fixed_freq)
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

void SimTransmitter2::common_frequency_update(Mode *mode)
{
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Note: mode is expected to be a VFO object
    VFO *vfo = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo->_frequency) + (vfo->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency = _vfo_freq - _fixed_freq;
      // Add BFO offset for comfortable audio tuning
    // This shifts the audio frequency without affecting signal meter calculations
    _frequency = raw_frequency + option_bfo_offset;

    // Note: mode is expected to be a VFO object
    VFO *vfo_c = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo_c->_frequency) + (vfo_c->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency_c = _vfo_freq - _fixed_freq;
      // Add BFO offset for comfortable audio tuning + test offset for dual generator verification
    // This shifts the audio frequency without affecting signal meter calculations
    _frequency_c = raw_frequency_c + option_bfo_offset + GENERATOR_C_TEST_OFFSET;
#elif defined(ENABLE_GENERATOR_A)
    // Note: mode is expected to be a VFO object
    VFO *vfo = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo->_frequency) + (vfo->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency = _vfo_freq - _fixed_freq;
      // Add BFO offset for comfortable audio tuning
    // This shifts the audio frequency without affecting signal meter calculations
    _frequency = raw_frequency + option_bfo_offset;
#elif defined(ENABLE_GENERATOR_C)
    // Note: mode is expected to be a VFO object
    VFO *vfo_c = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo_c->_frequency) + (vfo_c->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency_c = _vfo_freq - _fixed_freq;
      // Add BFO offset for comfortable audio tuning + test offset for dual generator verification
    // This shifts the audio frequency without affecting signal meter calculations
    _frequency_c = raw_frequency_c + option_bfo_offset + GENERATOR_C_TEST_OFFSET;
#endif
}

bool SimTransmitter2::check_frequency_bounds()
{
    // Check frequency bounds for all enabled generators
    bool any_in_bounds = false;

#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    if(_frequency > MAX_AUDIBLE_FREQ2 || _frequency < MIN_AUDIBLE_FREQ2){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ2, true);
                    wavegen->set_frequency(SILENT_FREQ2, false);
                }
            }
        }
    } else {
        any_in_bounds = true;
    }

    if(_frequency_c > MAX_AUDIBLE_FREQ2 || _frequency_c < MIN_AUDIBLE_FREQ2){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ2);
                }
            }
        }
    } else {
        any_in_bounds = true;
    }
#elif defined(ENABLE_GENERATOR_A)
    if(_frequency > MAX_AUDIBLE_FREQ2 || _frequency < MIN_AUDIBLE_FREQ2){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ2, true);
                    wavegen->set_frequency(SILENT_FREQ2, false);
                }
            }
        }
    } else {
        any_in_bounds = true;
    }
#elif defined(ENABLE_GENERATOR_C)
    if(_frequency_c > MAX_AUDIBLE_FREQ2 || _frequency_c < MIN_AUDIBLE_FREQ2){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ2);
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

void SimTransmitter2::end()
{
    // Call base class end() which properly handles the realizer cleanup
    Realization::end();
}

void SimTransmitter2::force_wave_generator_refresh()
{
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Force wave generator hardware update regardless of cached state
    // This is needed when returning to SimRadio after application switches
    if(_realizer != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->force_refresh();
    }
    // Force wave generator hardware update regardless of cached state
    // This is needed when returning to SimRadio after application switches
    if(_realizer != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(_realizer);
        wavegen_c->force_refresh();
    }
#elif defined(ENABLE_GENERATOR_A)
    // Force wave generator hardware update regardless of cached state
    // This is needed when returning to SimRadio after application switches
    if(_realizer != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->force_refresh();
    }
#elif defined(ENABLE_GENERATOR_C)
    // Force wave generator hardware update regardless of cached state
    // This is needed when returning to SimRadio after application switches
    if(_realizer != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(_realizer);
        wavegen_c->force_refresh();
    }
#endif
}

// Dynamic station management methods
bool SimTransmitter2::reinitialize(unsigned long time, float fixed_freq)
{
    // Reinitialize station with new frequency for dynamic management
    // This allows reusing dormant stations for new frequencies
    
    // Clean up any existing realizer assignment
    end();  // Safe to call multiple times
    
    // Set new shared frequency and reset shared state
    _fixed_freq = fixed_freq;
    _enabled = false;
    _active = false;
    
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Reset Wave Generator A state
    _frequency = 0.0;
    _station_state = ACTIVE2;  // Station is now active at new frequency

    // Reset Wave Generator C state
    _frequency_c = 0.0;
    // _station_state_c = ACTIVE2;  // Station is now active at new frequency
#elif defined(ENABLE_GENERATOR_A)
    // Reset Wave Generator A state
    _frequency = 0.0;
    _station_state = ACTIVE2;  // Station is now active at new frequency
#elif defined(ENABLE_GENERATOR_C)
    // Reset Wave Generator C state
    _frequency_c = 0.0;
    _station_state_c = ACTIVE2;  // Station is now active at new frequency
#endif
    
    // Start the station with the new frequency
    bool success = begin(time);
    
    // Subclasses should override this method to reinitialize their specific content
    // (e.g., new morse messages, different WPM, new pager content, etc.)
    
    return success;
}

void SimTransmitter2::randomize()
{
    // Default implementation: no randomization
    // Subclasses should override this to randomize their specific properties
    // (e.g., callsigns, WPM, fist quality, message content, etc.)
}

void SimTransmitter2::set_station_state(StationState2 new_state)
{
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    StationState2 old_state = _station_state;
    _station_state = new_state;
    
    // Handle state transition logic
    if(old_state == AUDIBLE2 && new_state != AUDIBLE2) {
        // Losing AD9833 generator - release it
        if(_realizer != -1) {
            end();  // This will free the realizer
        }
    }

    // StationState2 old_state_c = _station_state_c;
    // _station_state_c = new_state;
    
    // // Handle state transition logic
    // if(old_state_c == AUDIBLE2 && new_state != AUDIBLE2) {
    //     // Losing AD9833 generator - release it
    //     if(_realizer != -1) {
    //         end();  // This will free the realizer
    //     }
    // }
#elif defined(ENABLE_GENERATOR_A)
    StationState2 old_state = _station_state;
    _station_state = new_state;
    
    // Handle state transition logic
    if(old_state == AUDIBLE2 && new_state != AUDIBLE2) {
        // Losing AD9833 generator - release it
        if(_realizer != -1) {
            end();  // This will free the realizer
        }
    }
#elif defined(ENABLE_GENERATOR_C)
    StationState2 old_state_c = _station_state_c;
    _station_state_c = new_state;
    
    // Handle state transition logic
    if(old_state_c == AUDIBLE2 && new_state != AUDIBLE2) {
        // Losing AD9833 generator - release it
        if(_realizer != -1) {
            end();  // This will free the realizer
        }
    }
#endif

    // Note: Gaining AD9833 generator (ACTIVE/SILENT -> AUDIBLE) will be handled
    // by the StationManager when it assigns a realizer to this station
}

StationState2 SimTransmitter2::get_station_state() const
{
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    return _station_state;

    // return _station_state_c;
#elif defined(ENABLE_GENERATOR_A)
    return _station_state;
#elif defined(ENABLE_GENERATOR_C)
    return _station_state_c;
#endif
}

bool SimTransmitter2::is_audible() const
{
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    return _station_state == AUDIBLE2;
    
    // return _station_state_c == AUDIBLE2;
#elif defined(ENABLE_GENERATOR_A)
    return _station_state == AUDIBLE2;
#elif defined(ENABLE_GENERATOR_C)
    return _station_state_c == AUDIBLE2;
#endif
}

float SimTransmitter2::get_fixed_frequency() const
{
    return _fixed_freq;  // Return shared frequency
}

void SimTransmitter2::setActive(bool active) {
    _active = active;  // Use shared variable
}

bool SimTransmitter2::isActive() const {
    return _active;  // Use shared variable
}

void SimTransmitter2::force_frequency_update()
{
    // Immediately recalculate frequencies and update wave generators
    // This is used when _fixed_freq changes outside of the normal update() cycle
    // (e.g., frequency drift, dynamic station reallocation)
    // 
    // Without this, frequency changes would only take effect when the user turns
    // the tuning knob, causing the audio to stay at the old frequency while the
    // signal meter correctly shows the new frequency (confusing behavior).
    //
    // NOTE: This assumes the station currently holds all needed wave generators.
    
    if(!_enabled || !has_all_realizers()) {
        return;  // Skip if not enabled or missing realizers
    }
    
    int realizer_index = 0;  // Track which realizer to use
    
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Update Generator A
    float raw_frequency = _vfo_freq - _fixed_freq;
    _frequency = raw_frequency + option_bfo_offset;
    
    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer_a);
        wavegen->set_frequency(_frequency);
    }

    // Update Generator C (with test offset)
    float raw_frequency_c = _vfo_freq - _fixed_freq;
    _frequency_c = raw_frequency_c + option_bfo_offset + GENERATOR_C_TEST_OFFSET;
    
    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->set_frequency(_frequency_c);
    }
#elif defined(ENABLE_GENERATOR_A)
    // Update Generator A
    float raw_frequency = _vfo_freq - _fixed_freq;
    _frequency = raw_frequency + option_bfo_offset;
    
    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer_a);
        wavegen->set_frequency(_frequency);
    }
#elif defined(ENABLE_GENERATOR_C)
    // Update Generator C (with test offset)
    float raw_frequency_c = _vfo_freq - _fixed_freq;
    _frequency_c = raw_frequency_c + option_bfo_offset + GENERATOR_C_TEST_OFFSET;
    
    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->set_frequency(_frequency_c);
    }
#endif
}
