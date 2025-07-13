#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_rtty.h"
#include "signal_meter.h"

// Single authentic RTTY message for realistic station simulation
static const char rtty_message[] = "CQ CQ DE N6CCM K       ";

// mode is expected to be a derivative of VFO
SimRTTY::SimRTTY(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq) 
    : SimTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter)
{
    // Initialize message cycling state - start with initial MARK tone
    _in_wait_delay = true;
    _in_round_break = false;
    _in_initial_mark = true;  // Start each round with initial MARK tone
    _next_message_time = 0;  // Will be set properly in begin() with actual time
    _message_repeat_count = 3;  // Repeat each message 3 times for longer transmission
    _current_repeat = 0;
}

bool SimRTTY::begin(unsigned long time){
    if(!common_begin(time, _fixed_freq))
        return false;

    // Check if we have a valid realizer before accessing it
    if(_realizer == -1) {
        return false;
    }

    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);

    wavegen->set_frequency(SILENT_FREQ, false);
    wavegen->set_frequency(SILENT_FREQ, true);

    // Set up initial timing if this is the very first begin()
    if (_next_message_time == 0) {
        _next_message_time = time + (RTTY_MARK_TONE_SECONDS * 1000);  // Initial MARK tone
    }

    return true;
}

#define MAX_PHASE 36

void SimRTTY::realize(){
    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }
    
    // RESOURCE MANAGEMENT: Check if we have a wave generator
    if (_realizer == -1) {
        return;  // No resource available - station is dormant
    }
      WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    
    if(_in_round_break) {
        // During long silent period between rounds, force both channels to silent frequency
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
        wavegen->set_active_frequency(false);
    } else {
        // Normal RTTY operation or short MARK delay between repetitions
        wavegen->set_active_frequency(_active);
    }
}

// returns true on successful update
bool SimRTTY::update(Mode *mode){
    common_frequency_update(mode);    
    
    if(_enabled && _realizer != -1){  // RESOURCE MANAGEMENT: Check if we have a wave generator
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->set_frequency(_frequency, true);
        wavegen->set_frequency(_frequency + MARK_FREQ_SHIFT, false);
    }

    realize();

    return true;
}

// call periodically to keep realization dynamic
// returns true if it should keep going
bool SimRTTY::step(unsigned long time){
    // Handle state transitions for message cycling and idle patterns
    if (_in_wait_delay) {
        // During wait delays, behavior depends on type
        if (_in_round_break) {
            // Long break between rounds - keep transmitter completely silent
            _active = false;
        } else {
            // Short break between repetitions or initial MARK - keep MARK tone (carrier on)
            _active = true;
        }
        realize();
        
        // Check if wait period is over
        if (time >= _next_message_time) {
            _in_wait_delay = false;
            
            // Check if we were in the initial MARK tone phase
            if (_in_initial_mark) {
                _in_initial_mark = false;
                // After initial MARK, start the first message
                _rtty.start_rtty_message(rtty_message, false);
                return true;
            }
            
            // Check if we were in the final MARK tone phase
            if (!_in_round_break && _current_repeat > _message_repeat_count) {
                // Transition from final MARK tone to long silent period
                _active = false;  // Turn off transmitter for silent period
                realize();
                
                // RESOURCE MANAGEMENT: Release wave generator during silent period
                // First ensure frequencies are silenced
                if(_realizer != -1) {
                    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                    wavegen->set_frequency(SILENT_FREQ, true);
                    wavegen->set_frequency(SILENT_FREQ, false);
                    wavegen->set_active_frequency(false);
                }
                // Release the resource for other stations to use
                end();  // This calls Realization::end() which frees the realizer
                
                _in_wait_delay = true;
                _in_round_break = true;  // Now enter the long silent delay between rounds
                _next_message_time = time + (RTTY_WAIT_SECONDS * 1000);
                // Reset repeat count for next round
                _current_repeat = 0;
                return true;
            }
            
            _in_round_break = false;
            
            // Restore MARK and SPACE frequencies if they were overwritten during round break
            // Note: This happens when we already have a realizer and just exited round break
            if(_enabled && _realizer != -1){
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(_frequency, true);  // SPACE frequency (channel 1)
                wavegen->set_frequency(_frequency + MARK_FREQ_SHIFT, false);  // MARK frequency (channel 0)
            }
            
            // Start next message after wait delay completes (but set up initial MARK for new round)
            if (_current_repeat == 0) {
                // Beginning of new round - try to reacquire wave generator resource
                if (_realizer == -1) {
                    // RESOURCE MANAGEMENT: Attempt to reacquire wave generator
                    if (!begin(time)) {
                        // No resource available - remain dormant and try again later
                        _in_wait_delay = true;
                        _in_round_break = true;  // Stay in dormant state
                        _next_message_time = time + 1000;  // Check again in 1 second
                        return true;
                    }
                    // Successfully acquired resource - force frequency update like other station types
                    force_frequency_update();  // This sets up the base frequency
                    // RTTY also needs the MARK frequency set up
                    if(_enabled && _realizer != -1) {
                        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                        wavegen->set_frequency(_frequency, true);  // SPACE frequency
                        wavegen->set_frequency(_frequency + MARK_FREQ_SHIFT, false);  // MARK frequency
                    }
                }
                
                // Beginning of new round - start with initial MARK tone
                _in_wait_delay = true;
                _in_initial_mark = true;
                _next_message_time = time + (RTTY_MARK_TONE_SECONDS * 1000);  // Initial MARK tone
            } else {
                // Continue with next message in current round
                _rtty.start_rtty_message(rtty_message, false);
            }
        }
        return true;
    }
    
    // Process RTTY state machine only when not in wait delay and we have a wave generator
    if (_realizer != -1) {  // RESOURCE MANAGEMENT: Only process when we have a resource
        switch(_rtty.step_rtty(time)){
        	case STEP_RTTY_TURN_ON:
                _active = true;
                realize();
                send_carrier_charge_pulse(_signal_meter);  // Send charge pulse when carrier turns on
        		break;

        	case STEP_RTTY_LEAVE_ON:
                // Carrier remains on - send another charge pulse
                send_carrier_charge_pulse(_signal_meter);
                break;

        	case STEP_RTTY_TURN_OFF:
                _active = false;
                realize();
                // No charge pulse when carrier turns off
        		break;
        }
    }

    // Check for message completion to trigger state transitions
    if (_rtty.is_message_complete()) {
        if (!_in_wait_delay) {
            // Regular message finished - check if we need more repetitions            
            _current_repeat++;
            if (_current_repeat < _message_repeat_count) {
                // Send the same message again after a brief delay with MARK tone
                _in_wait_delay = true;
                _in_round_break = false;  // Short delay between repetitions (MARK tone)
                _next_message_time = time + (RTTY_MARK_TONE_SECONDS * 1000);  // MARK tone between repetitions
            } else {
                // All repetitions done, but first add final MARK tone period before silent wait
                _in_wait_delay = true;
                _in_round_break = false;  // Final MARK tone period (not silent yet)
                _next_message_time = time + (RTTY_MARK_TONE_SECONDS * 1000);  // Final MARK tone
                // Mark that we need to go to silent period after this MARK tone
                _current_repeat++;  // Increment to indicate we're in the final MARK phase
            }
        }
    }

    return true;
}
