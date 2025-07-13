#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_pager.h"
#include "signal_meter.h"

SimPager::SimPager(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq) 
    : SimTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter)
{
    // Generate initial tone pair
    generate_new_tone_pair();
    // Pager transmission will be started in begin() method
}

bool SimPager::begin(unsigned long time)
{
    if(!common_begin(time, _fixed_freq))
        return false;// Start pager transmission with repeat enabled
    _pager.start_pager_transmission(true);

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

void SimPager::realize()
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
        // Set frequencies based on current pager state
        switch(_pager.get_current_state()) {
            case PAGER_STATE_TONE_A:
                // Transmit Tone A (longer unsquelch tone)
                wavegen->set_frequency(_frequency + _current_tone_a_offset, true);
                wavegen->set_frequency(_frequency + _current_tone_a_offset, false);
                break;
                
            case PAGER_STATE_TONE_B:
                // Transmit Tone B (shorter identification tone)
                wavegen->set_frequency(_frequency + _current_tone_b_offset, true);
                wavegen->set_frequency(_frequency + _current_tone_b_offset, false);
                break;
                  default:
                // Silent state (SILENCE) - should not reach here when _active is true
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                break;
        }    } else {
        // Explicitly set silent frequencies when inactive (during SILENCE)
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
    }
    
    wavegen->set_active_frequency(_active);
}

bool SimPager::update(Mode *mode)
{
    common_frequency_update(mode);

    if(_enabled) {
        // Note: We don't set frequencies here like RTTY does
        // Pager frequencies are set in realize() based on current tone
    }

    realize();
    return true;
}

bool SimPager::step(unsigned long time)
{
    switch(_pager.step_pager(time)) {        case STEP_PAGER_TURN_ON:
            // Check if this is the start of a new page cycle (silence → tone A)
            if (_pager.get_current_state() == PAGER_STATE_TONE_A) {
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

        case STEP_PAGER_LEAVE_ON:
            // Carrier remains on - send another charge pulse
            send_carrier_charge_pulse(_signal_meter);
            break;

        case STEP_PAGER_TURN_OFF:
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
              case STEP_PAGER_CHANGE_FREQ:
            // Transmitter stays on, but frequency needs to change
            realize();
            send_carrier_charge_pulse(_signal_meter);  // Send charge pulse on frequency change while on
            break;
            
        // LEAVE_ON and LEAVE_OFF don't require action since _active state doesn't change
        // and no frequency update is needed during silence
    }

    return true;
}

void SimPager::generate_new_tone_pair()
{
    // Generate random tone pair similar to DTMF frequencies
    // Range: 650-1650 Hz offset, minimum 200 Hz separation
    
    float frequency_range = PAGER_TONE_MAX_OFFSET - PAGER_TONE_MIN_OFFSET;
    
    // Arduino random() function
    _current_tone_a_offset = PAGER_TONE_MIN_OFFSET + 
        random((long)(frequency_range - PAGER_TONE_MIN_SEPARATION));
    
    // Generate second tone with minimum separation
    float remaining_range = frequency_range - PAGER_TONE_MIN_SEPARATION;
    float tone_b_base = random((long)remaining_range);
    
    // Ensure minimum separation
    if (tone_b_base < _current_tone_a_offset - PAGER_TONE_MIN_OFFSET) {
        _current_tone_b_offset = PAGER_TONE_MIN_OFFSET + tone_b_base;
    } else {
        _current_tone_b_offset = _current_tone_a_offset + PAGER_TONE_MIN_SEPARATION + 
            (tone_b_base - (_current_tone_a_offset - PAGER_TONE_MIN_OFFSET));
    }

    // Ensure tone B doesn't exceed maximum
    if (_current_tone_b_offset > PAGER_TONE_MAX_OFFSET) {
        _current_tone_b_offset = PAGER_TONE_MAX_OFFSET;
    }
}

void SimPager::debug_print_tone_pair() const
{
    // Debug output not needed for Arduino build
}
