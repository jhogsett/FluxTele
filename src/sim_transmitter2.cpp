#include "sim_transmitter2.h"
#include "wavegen.h"
#include "vfo.h"
#include "saved_data.h"  // For option_bfo_offset

SimTransmitter2::SimTransmitter2(WaveGenPool *wave_gen_pool, float fixed_freq) 
    : Realization(wave_gen_pool, (int)(fixed_freq / 1000))  // Pass frequency in kHz as station ID
{
#ifdef ENABLE_GENERATOR_A
    // Initialize common member variables
    _fixed_freq = fixed_freq;
    _enabled = false;
    _frequency = 0.0;
    _active = false;
    _vfo_freq = 0.0;  // Initialize VFO frequency to prevent garbage values
    
    // Initialize dynamic station management state
    _station_state = DORMANT2;
#endif
#ifdef ENABLE_GENERATOR_B
    // Initialize common member variables
    _fixed_freq_b = fixed_freq;
    _enabled_b = false;
    _frequency_b = 0.0;
    _active_b = false;
    _vfo_freq_b = 0.0;  // Initialize VFO frequency to prevent garbage values
    
    // Initialize dynamic station management state
    _station_state_b = DORMANT2;
#endif
}

bool SimTransmitter2::common_begin(unsigned long time, float fixed_freq)
{
#ifdef ENABLE_GENERATOR_A
    _fixed_freq = fixed_freq;
    _frequency = 0.0;
    
    // Update station ID for debugging (frequency in kHz)
    set_station_id((int)(fixed_freq / 1000));
    
    return Realization::begin(time);
#endif
#ifdef ENABLE_GENERATOR_B
    _fixed_freq_b = fixed_freq;
    _frequency_b = 0.0;
    
    // Update station ID for debugging (frequency in kHz)
    set_station_id((int)(fixed_freq / 1000));
    
    return Realization::begin(time);
#endif
}

void SimTransmitter2::common_frequency_update(Mode *mode)
{
#ifdef ENABLE_GENERATOR_A
    // Note: mode is expected to be a VFO object
    VFO *vfo = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo->_frequency) + (vfo->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency = _vfo_freq - _fixed_freq;
      // Add BFO offset for comfortable audio tuning
    // This shifts the audio frequency without affecting signal meter calculations
    _frequency = raw_frequency + option_bfo_offset;
#endif
#ifdef ENABLE_GENERATOR_B
    // Note: mode is expected to be a VFO object
    VFO *vfo_b = static_cast<VFO*>(mode);
    _vfo_freq_b = float(vfo_b->_frequency) + (vfo_b->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency_b = _vfo_freq_b - _fixed_freq_b;
      // Add BFO offset for comfortable audio tuning
    // This shifts the audio frequency without affecting signal meter calculations
    _frequency_b = raw_frequency_b + option_bfo_offset;
#endif
}

bool SimTransmitter2::check_frequency_bounds()
{
#ifdef ENABLE_GENERATOR_A
    if(_frequency > MAX_AUDIBLE_FREQ2 || _frequency < MIN_AUDIBLE_FREQ2){
        if(_enabled){
            _enabled = false;

            // Check if we have a valid realizer before accessing it
            if(_realizer != -1) {
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(SILENT_FREQ2, true);
                wavegen->set_frequency(SILENT_FREQ2, false);
            }
        }
        return false;  // Out of bounds
    } 
        
    if(!_enabled){
        _enabled = true;
    }
      return true;  // In bounds
#endif
#ifdef ENABLE_GENERATOR_B
    if(_frequency_b > MAX_AUDIBLE_FREQ2 || _frequency_b < MIN_AUDIBLE_FREQ2){
        if(_enabled_b){
            _enabled_b = false;

            // Check if we have a valid realizer before accessing it
            if(_realizer != -1) {
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(SILENT_FREQ2, true);
                wavegen->set_frequency(SILENT_FREQ2, false);
            }
        }
        return false;  // Out of bounds
    } 
        
    if(!_enabled_b){
        _enabled_b = true;
    }
      return true;  // In bounds
#endif
}

void SimTransmitter2::end()
{
    // Call base class end() which properly handles the realizer cleanup
    Realization::end();
}

void SimTransmitter2::force_wave_generator_refresh()
{
#ifdef ENABLE_GENERATOR_A
    // Force wave generator hardware update regardless of cached state
    // This is needed when returning to SimRadio after application switches
    if(_realizer != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->force_refresh();
    }
#endif
#ifdef ENABLE_GENERATOR_B
    // Force wave generator hardware update regardless of cached state
    // This is needed when returning to SimRadio after application switches
    if(_realizer != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->force_refresh();
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
    
#ifdef ENABLE_GENERATOR_A
    // Set new parameters
    _fixed_freq = fixed_freq;
    _frequency = 0.0;
    _enabled = false;
    _active = false;
    _station_state = ACTIVE2;  // Station is now active at new frequency
#endif
#ifdef ENABLE_GENERATOR_B
    // Set new parameters
    _fixed_freq_b = fixed_freq;
    _frequency_b = 0.0;
    _enabled_b = false;
    _active_b = false;
    _station_state_b = ACTIVE2;  // Station is now active at new frequency
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
#ifdef ENABLE_GENERATOR_A
    StationState2 old_state = _station_state;
    _station_state = new_state;
    
    // Handle state transition logic
    if(old_state == AUDIBLE2 && new_state != AUDIBLE2) {
        // Losing AD9833 generator - release it
        if(_realizer != -1) {
            end();  // This will free the realizer
        }
    }
#endif
#ifdef ENABLE_GENERATOR_B
    StationState2 old_state_b = _station_state_b;
    _station_state_b = new_state;
    
    // Handle state transition logic
    if(old_state_b == AUDIBLE2 && new_state != AUDIBLE2) {
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
#ifdef ENABLE_GENERATOR_A
    return _station_state;
#endif
#ifdef ENABLE_GENERATOR_B
    return _station_state_b;
#endif
}

bool SimTransmitter2::is_audible() const
{
#ifdef ENABLE_GENERATOR_A
    return _station_state == AUDIBLE2;
#endif
#ifdef ENABLE_GENERATOR_B
    return _station_state_b == AUDIBLE2;
#endif
}

float SimTransmitter2::get_fixed_frequency() const
{
#ifdef ENABLE_GENERATOR_A
    return _fixed_freq;
#endif
#ifdef ENABLE_GENERATOR_B
    return _fixed_freq_b;
#endif
}

void SimTransmitter2::setActive(bool active) {
#ifdef ENABLE_GENERATOR_A
    _active = active;
#endif
#ifdef ENABLE_GENERATOR_B
    _active_b = active;
#endif
}

bool SimTransmitter2::isActive() const {
#ifdef ENABLE_GENERATOR_A
    return _active;
#endif
#ifdef ENABLE_GENERATOR_B
    return _active_b;
#endif
}

void SimTransmitter2::force_frequency_update()
{
#ifdef ENABLE_GENERATOR_A
    // Immediately recalculate _frequency and update wave generator
    // This is used when _fixed_freq changes outside of the normal update() cycle
    // (e.g., frequency drift, dynamic station reallocation)
    // 
    // Without this, frequency changes would only take effect when the user turns
    // the tuning knob, causing the audio to stay at the old frequency while the
    // signal meter correctly shows the new frequency (confusing behavior).
    //
    // NOTE: This assumes the station currently holds a wave generator. When full
    // dynamic pipelining is implemented (wave generators allocated/freed based on
    // VFO proximity), this method should be made more defensive and frequency
    // changes should be deferred until a generator is re-allocated.
    if(_enabled && _realizer != -1) {
        // Recalculate _frequency with current _fixed_freq and _vfo_freq
        float raw_frequency = _vfo_freq - _fixed_freq;
        _frequency = raw_frequency + option_bfo_offset;
          // Update the wave generator with the new frequency
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->set_frequency(_frequency);
    }
#endif
#ifdef ENABLE_GENERATOR_B
    // Immediately recalculate _frequency and update wave generator
    // This is used when _fixed_freq changes outside of the normal update() cycle
    // (e.g., frequency drift, dynamic station reallocation)
    // 
    // Without this, frequency changes would only take effect when the user turns
    // the tuning knob, causing the audio to stay at the old frequency while the
    // signal meter correctly shows the new frequency (confusing behavior).
    //
    // NOTE: This assumes the station currently holds a wave generator. When full
    // dynamic pipelining is implemented (wave generators allocated/freed based on
    // VFO proximity), this method should be made more defensive and frequency
    // changes should be deferred until a generator is re-allocated.
    if(_enabled_b && _realizer != -1) {
        // Recalculate _frequency with current _fixed_freq and _vfo_freq
        float raw_frequency_b = _vfo_freq_b - _fixed_freq_b;
        _frequency_b = raw_frequency_b + option_bfo_offset;
          // Update the wave generator with the new frequency
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->set_frequency(_frequency_b);
    }
#endif
}
