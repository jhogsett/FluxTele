#include "sim_dualtone.h"
#include "wavegen.h"
#include "vfo.h"
#include "saved_data.h"

SimDualTone::SimDualTone(WaveGenPool *wave_gen_pool, float fixed_freq) 
    : Realization(wave_gen_pool, (int)(fixed_freq / 1000), 
        2  // Dual generator mode requires 2 realizers
    )
{
    // Initialize shared station properties
    _fixed_freq = fixed_freq;
    _enabled = false;
    _active = false;
    _vfo_freq = 0.0;  // Initialize VFO frequency to prevent garbage values

    // Initialize Wave Generators variables
    _frequency = 0.0;
    _frequency2 = 0.0;
    
    // Initialize dynamic station management state
    _station_state = DORMANT;
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
    _frequency = 0.0;
    _frequency2 = 0.0;
    
    return true;
}

void SimDualTone::common_frequency_update(Mode *mode)
{
    // Note: mode is expected to be a VFO object
    VFO *vfo = static_cast<VFO*>(mode);
    _vfo_freq = float(vfo->_frequency) + (vfo->_sub_frequency / 10.0);
    
    // Calculate raw frequency difference (used for signal meter - no BFO offset)
    float raw_frequency = _vfo_freq - _fixed_freq;

      // Add BFO offset for comfortable audio tuning
    // This shifts the audio frequency without affecting signal meter calculations
    _frequency = raw_frequency + option_bfo_offset + getFrequencyOffsetA();
    _frequency2 = raw_frequency + option_bfo_offset + getFrequencyOffsetC();
}

bool SimDualTone::check_frequency_bounds()
{
    // Check frequency bounds for all enabled generators
    bool any_in_bounds = false;

    if(_frequency > MAX_AUDIBLE_FREQ || _frequency < MIN_AUDIBLE_FREQ){
        if(_enabled){
            _enabled = false;
            // Set all generators to silent frequency
            for(int i = 0; i < get_realizer_count(); i++) {
                int realizer = get_realizer(i);
                if(realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
                    wavegen->set_frequency(SILENT_FREQ, true);
                    wavegen->set_frequency(SILENT_FREQ, false);
                }
            }
        }
    } else {
        any_in_bounds = true;
    }

    if(any_in_bounds && !_enabled){
        _enabled = true;
    }
    
    return any_in_bounds;
}

void SimDualTone::end()
{
    // Call base class end() which properly handles the realizer cleanup
    Realization::end();
}

void SimDualTone::force_wave_generator_refresh()
{
    // Initialize all acquired wave generators
    int realizer_index = 0;
    
    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer_a);
        wavegen->force_refresh();
    }

    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->force_refresh();
    }
}

// Dynamic station management methods
bool SimDualTone::reinitialize(unsigned long time, float fixed_freq)
{
    // Reinitialize station with new frequency for dynamic management
    // This allows reusing dormant stations for new frequencies
    
    // Clean up any existing realizer assignment
    end();  // Safe to call multiple times
    
    // Set new shared frequency and reset shared state
    _fixed_freq = fixed_freq;
    _enabled = false;
    _active = false;
    
    // Reset Wave Generatorz statez
    _frequency = 0.0;
    _frequency2 = 0.0;

    _station_state = ACTIVE;  // Station is now active at new frequency
    
    // Start the station with the new frequency
    bool success = begin(time);
    
    // Subclasses should override this method to reinitialize their specific content
    // (e.g., new morse messages, different WPM, new pager content, etc.)
    
    return success;
}

void SimDualTone::randomize()
{
    // Default implementation: no randomization
    // Subclasses should override this to randomize their specific properties
    // (e.g., callsigns, WPM, fist quality, message content, etc.)
}

void SimDualTone::set_station_state(StationState new_state)
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
}

StationState SimDualTone::get_station_state() const
{
    return _station_state;
}

bool SimDualTone::is_audible() const
{
    return _station_state == AUDIBLE;
}

float SimDualTone::get_fixed_frequency() const
{
    return _fixed_freq;  // Return shared frequency
}

void SimDualTone::setActive(bool active) {
    _active = active;  // Use shared variable
}

bool SimDualTone::isActive() const {
    return _active;  // Use shared variable
}

void SimDualTone::force_frequency_update()
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
    
    if(!_active)
        return;

        int realizer_index = 0;  // Track which realizer to use
    
    
    float raw_frequency = _vfo_freq - _fixed_freq;
    _frequency = raw_frequency + option_bfo_offset + getFrequencyOffsetA();
   
    int realizer_a = get_realizer(realizer_index++);
    if(realizer_a != -1) {
        WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer_a);
        wavegen->set_frequency(_frequency);
    }
    
    float raw_frequency2 = _vfo_freq - _fixed_freq;
    _frequency2 = raw_frequency2 + option_bfo_offset + getFrequencyOffsetC();

    int realizer_c = get_realizer(realizer_index++);
    if(realizer_c != -1) {
        WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
        wavegen_c->set_frequency(_frequency2);
    }
}

// void SimDualTone::force_frequency_update2()
// {
//     // // Immediately recalculate frequencies and update wave generators
//     // // This is used when _fixed_freq changes outside of the normal update() cycle
//     // // (e.g., frequency drift, dynamic station reallocation)
//     // // 
//     // // Without this, frequency changes would only take effect when the user turns
//     // // the tuning knob, causing the audio to stay at the old frequency while the
//     // // signal meter correctly shows the new frequency (confusing behavior).
//     // //
//     // // NOTE: This assumes the station currently holds all needed wave generators.

//     Serial.println("ffu2-----------");
    
//     // if(!_enabled){
//     //     Serial.println("ffu2 not enabled");
//     // }

//     // if(!has_all_realizers()){
//     //     Serial.println("ffu2 not all realizers");
//     // }

//     // if(!_active){
//     //     Serial.println("ffu2 not active");
//     // }
    
//     // if(!_enabled || !has_all_realizers()) {
//     //     return;  // Skip if not enabled or missing realizers
//     // }
    
//     int realizer_index = 0;  // Track which realizer to use
    
//     // if(!_active)
//     //     return;

//     // Serial.println("ffu-----------");
    
//     float raw_frequency = _vfo_freq - _fixed_freq;
//     _frequency = raw_frequency + option_bfo_offset + getFrequencyOffsetA();
   
//     // Serial.println(_frequency);
    
//     int realizer_a = get_realizer(realizer_index++);
//     if(realizer_a != -1) {
//         WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer_a);
//         wavegen->set_frequency(_frequency);
//         Serial.println("this ruins it 1");
//         Serial.println(_frequency);
//     }
    
//     float raw_frequency2 = _vfo_freq - _fixed_freq;
//     _frequency2 = raw_frequency2 + option_bfo_offset + getFrequencyOffsetC();

//     // Serial.println(_frequency2);
    
//     int realizer_c = get_realizer(realizer_index++);
//     if(realizer_c != -1) {
//         WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
//         wavegen_c->set_frequency(_frequency2);
//         Serial.println("this ruins it 2");
//         Serial.println(_frequency2);
//     }
// }

// Centralized charge pulse logic for all simulated stations
void SimDualTone::send_carrier_charge_pulse(SignalMeter* signal_meter) {
    if (!signal_meter) return;
    int charge = VFO::calculate_signal_charge(_fixed_freq, _vfo_freq);
    if (charge > 0) {
        const float LOCK_WINDOW_HZ = 50.0; // Lock window threshold (adjust as needed)
        float freq_diff = abs(_fixed_freq - _vfo_freq);
        if (freq_diff <= LOCK_WINDOW_HZ) {
            signal_meter->add_charge(-charge);
        } else {
            signal_meter->add_charge(charge);
        }
    }
}

// Virtual methods for frequency offsets (default implementation uses macros)
float SimDualTone::getFrequencyOffsetA() const {
    return GENERATOR_A_TEST_OFFSET;  // Default to macro for backward compatibility
}

float SimDualTone::getFrequencyOffsetC() const {
    return GENERATOR_C_TEST_OFFSET;  // Default to macro for backward compatibility
}
