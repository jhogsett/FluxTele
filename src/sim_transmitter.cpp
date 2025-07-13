#include "sim_transmitter.h"
#include "wavegen.h"
#include "vfo.h"
#include "saved_data.h"  // For option_bfo_offset

SimTransmitter::SimTransmitter(WaveGenPool *wave_gen_pool, float fixed_freq) 
    : Realization(wave_gen_pool, (int)(fixed_freq / 1000))  // Pass frequency in kHz as station ID
{
    // Initialize common member variables
    _fixed_freq = fixed_freq;
    _enabled = false;
    _frequency = 0.0;
    _active = false;
    _vfo_freq = 0.0;  // Initialize VFO frequency to prevent garbage values
    
    // Initialize dynamic station management state
    _station_state = DORMANT;
}

bool SimTransmitter::common_begin(unsigned long time, float fixed_freq)
{
    _fixed_freq = fixed_freq;
    _frequency = 0.0;
    
    // Update station ID for debugging (frequency in kHz)
    set_station_id((int)(fixed_freq / 1000));
    
    return Realization::begin(time);
}

void SimTransmitter::common_frequency_update(Mode *mode)
{
    // Note: mode is expected to be a VFO object
    VFO *vfo = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo->_frequency) + (vfo->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency = _vfo_freq - _fixed_freq;
      // Add BFO offset for comfortable audio tuning
    // This shifts the audio frequency without affecting signal meter calculations
    _frequency = raw_frequency + option_bfo_offset;
}

bool SimTransmitter::check_frequency_bounds()
{
    if(_frequency > MAX_AUDIBLE_FREQ || _frequency < MIN_AUDIBLE_FREQ){
        if(_enabled){
            _enabled = false;

            // Check if we have a valid realizer before accessing it
            if(_realizer != -1) {
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
            }
        }
        return false;  // Out of bounds
    } 
        
    if(!_enabled){
        _enabled = true;
    }
      return true;  // In bounds
}

void SimTransmitter::end()
{
    // Call base class end() which properly handles the realizer cleanup
    Realization::end();
}

void SimTransmitter::force_wave_generator_refresh()
{    // Force wave generator hardware update regardless of cached state
    // This is needed when returning to SimRadio after application switches
    if(_realizer != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->force_refresh();
    }
}

// Dynamic station management methods
bool SimTransmitter::reinitialize(unsigned long time, float fixed_freq)
{
    // Reinitialize station with new frequency for dynamic management
    // This allows reusing dormant stations for new frequencies
    
    // Clean up any existing realizer assignment
    end();  // Safe to call multiple times
    
    // Set new parameters
    _fixed_freq = fixed_freq;
    _frequency = 0.0;
    _enabled = false;
    _active = false;
    _station_state = ACTIVE;  // Station is now active at new frequency
    
    // Start the station with the new frequency
    bool success = begin(time);
    
    // Subclasses should override this method to reinitialize their specific content
    // (e.g., new morse messages, different WPM, new pager content, etc.)
    
    return success;
}

void SimTransmitter::randomize()
{
    // Default implementation: no randomization
    // Subclasses should override this to randomize their specific properties
    // (e.g., callsigns, WPM, fist quality, message content, etc.)
}

void SimTransmitter::set_station_state(StationState new_state)
{
    StationState old_state = _station_state;
    _station_state = new_state;
    
    // Handle state transition logic
    if(old_state == AUDIBLE && new_state != AUDIBLE) {
        // Losing AD9833 generator - release it
        if(_realizer != -1) {
            end();  // This will free the realizer
        }
    }
    // Note: Gaining AD9833 generator (ACTIVE/SILENT -> AUDIBLE) will be handled
    // by the StationManager when it assigns a realizer to this station
}

StationState SimTransmitter::get_station_state() const
{
    return _station_state;
}

bool SimTransmitter::is_audible() const
{
    return _station_state == AUDIBLE;
}

float SimTransmitter::get_fixed_frequency() const
{
    return _fixed_freq;
}

void SimTransmitter::setActive(bool active) {
    _active = active;
}

bool SimTransmitter::isActive() const {
    return _active;
}

void SimTransmitter::force_frequency_update()
{
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
}