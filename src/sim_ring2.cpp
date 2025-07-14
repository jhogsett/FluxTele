#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_ring2.h"
#include "signal_meter.h"

SimRing2::SimRing2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq) 
    : SimTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter)
{
    // Generate initial tone pair
    generate_new_tone_pair();
    // Ring transmission will be started in begin() method
}

bool SimRing2::begin(unsigned long time)
{
    if(!common_begin(time, _fixed_freq))
        return false;
        
    // Start ring transmission with repeat enabled
    _telco.start_telco_transmission(true);

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

void SimRing2::realize()
{
    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }
    
    // Don't try to access wave generator if we don't have one (during silence)
    if(_realizer == -1) {
        return;
    }
    
    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    
    if(_active) {
        // Set frequencies based on current ring state
        switch(_telco.get_current_state()) {
            case TELCO_STATE_TONE_A:
                // Transmit Tone A (longer unsquelch tone)
                wavegen->set_frequency(_frequency + _current_tone_a_offset, true);
                wavegen->set_frequency(_frequency + _current_tone_a_offset, false);
                break;
                
            case TELCO_STATE_TONE_B:
                // Transmit Tone B (shorter identification tone)
                wavegen->set_frequency(_frequency + _current_tone_b_offset, true);
                wavegen->set_frequency(_frequency + _current_tone_b_offset, false);
                break;
                
            default:
                // Silent state (SILENCE) - should not reach here when _active is true
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                break;
        }
    } else {
        // Explicitly set silent frequencies when inactive (during SILENCE)
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
    }
    
    wavegen->set_active_frequency(_active);
}

bool SimRing2::update(Mode *mode)
{
    common_frequency_update(mode);

    if(_enabled && _realizer != -1 && !_active) {
        // Only update base frequency when not actively transmitting to avoid pops
        // During transmission, realize() handles frequency with tone offsets
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->set_frequency(_frequency);
    }

    realize();
    return true;
}

bool SimRing2::step(unsigned long time)
{
    switch(_telco.step_telco(time)) {
        case STEP_TELCO_TURN_ON:
            // Check if this is the start of a new page cycle (silence → tone A)
            if (_telco.get_current_state() == TELCO_STATE_TONE_A) {
                generate_new_tone_pair();
                
                // RESOURCE MANAGEMENT: Acquire wave generator after silent period
                // Need to get a new realizer if we freed it during silence
                if(_realizer == -1) {
                    if(!common_begin(time, _fixed_freq)) {
                        // Failed to get wave generator - stay inactive
                        _active = false;
                        return true;
                    }
                    // CRITICAL: Force frequency update after reacquiring generator
                    force_frequency_update();
                }
            }
            _active = true;
            realize();
            send_carrier_charge_pulse(_signal_meter);  // Send charge pulse when carrier turns on
            break;

        case STEP_TELCO_LEAVE_ON:
            // Carrier remains on - send another charge pulse
            send_carrier_charge_pulse(_signal_meter);
            break;

        case STEP_TELCO_TURN_OFF:
            _active = false;
            realize();
            
            // RESOURCE MANAGEMENT: Release wave generator during silent period
            // First ensure frequencies are silenced
            if(_realizer != -1) {
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                wavegen->set_active_frequency(false);
            }
            // Release the resource for other stations to use during silence
            end();  // This calls Realization::end() which frees the realizer
            
            // No charge pulse when carrier turns off
            break;
            
        case STEP_TELCO_CHANGE_FREQ:
            // Transmitter stays on, but frequency needs to change
            realize();
            send_carrier_charge_pulse(_signal_meter);  // Send charge pulse on frequency change while on
            break;
            
        // LEAVE_ON and LEAVE_OFF don't require action since _active state doesn't change
        // and no frequency update is needed during silence
    }

    return true;
}

void SimRing2::generate_new_tone_pair()
{
    // Fixed North American telephone ring frequencies
    // For single-tone ring: Generator 1 = 440 Hz (we'll use tone A for the single generator)
    
    _current_tone_a_offset = RING2_TONE_LOW_OFFSET;   // 440 Hz
    _current_tone_b_offset = RING2_TONE_HIGH_OFFSET;  // 480 Hz (not used in single generator mode)
}

void SimRing2::debug_print_tone_pair() const
{
    // Debug output not needed for Arduino build
}
