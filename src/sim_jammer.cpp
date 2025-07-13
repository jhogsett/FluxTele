#include "vfo.h"
#include "wavegen.h"
#include "realization_pool.h"
#include "sim_jammer.h"

SimJammer::SimJammer(WaveGenPool *wave_gen_pool) : SimTransmitter(wave_gen_pool, 0.0)  // Default freq, will be set in begin()
{
    // Base class initializes all common variables
    // Jammer transmission will be started in begin() method
}

bool SimJammer::begin(unsigned long time, float fixed_freq)
{
    if(!common_begin(time, fixed_freq))
        return false;

    // Start jammer transmission with repeat enabled (jammers run continuously)
    _jammer.start_jammer_transmission(true);

    // Check if we have a valid realizer before accessing it
    if(_realizer == -1) {
        return false;  // No realizer available
    }

    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);

    // Initialize both channels to silent
    wavegen->set_frequency(SILENT_FREQ, false);
    wavegen->set_frequency(SILENT_FREQ, true);

    return true;
}

void SimJammer::realize()
{
    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }
    
    // Check if we have a valid realizer before accessing it
    if(_realizer == -1) {
        return;  // No realizer available
    }

    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    
    if(_active && _jammer.get_current_state() == JAMMER_STATE_TRANSMITTING) {
        // Calculate current jamming frequency
        float jamming_frequency = _frequency + _jammer.get_frequency_offset();
        
        // Set jamming frequency on both channels
        wavegen->set_frequency(jamming_frequency, true);
        wavegen->set_frequency(jamming_frequency, false);
    } else {
        // Silent when inactive or muted
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
    }
    
    wavegen->set_active_frequency(_active && _jammer.get_current_state() == JAMMER_STATE_TRANSMITTING);
}

bool SimJammer::update(Mode *mode)
{
    common_frequency_update(mode);

    if(_enabled) {
        // Frequency is dynamically set in realize() based on drift
        // No fixed frequency setup needed here like RTTY
    }

    realize();
    return true;
}

bool SimJammer::step(unsigned long time)
{
    switch(_jammer.step_jammer(time)) {
        case STEP_JAMMER_TURN_ON:
            _active = true;
            realize();
            break;

        case STEP_JAMMER_TURN_OFF:
            _active = false;
            realize();
            break;
            
        case STEP_JAMMER_CHANGE_FREQ:
            // Transmitter stays on, but frequency changes due to drift
            realize();
            break;
            
        // LEAVE_ON and LEAVE_OFF don't require action since _active state doesn't change
        // and frequency drift is handled internally by AsyncJammer
    }

    return true;
}

