#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_numbers.h"
#include "signal_meter.h"
#include <Arduino.h>

#define INTER_GROUP_DELAY 2000  // 2 seconds delay between number groups (more distinct)
#define INTER_CYCLE_DELAY 8000  // 8 seconds delay between complete cycles

SimNumbers::SimNumbers(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq, int wpm) 
    : SimTransmitter(wave_gen_pool, fixed_freq), _wpm(wpm), _signal_meter(signal_meter)
{
    // Base class initializes all common variables, including _fixed_freq
    _groups_sent = 0;
    _total_groups_per_cycle = 13;  // 13 groups for creepiness
    _in_inter_group_delay = false;
    _next_group_time = 0;
    _transmission_active = false;
    
    // Enhanced numbers station state
    _current_phase = PHASE_INTERVAL_SIGNAL;
    _interval_repeats_sent = 0;
    _total_interval_repeats = DEFAULT_INTERVAL_REPEATS;  // Default interval repeats for optimal anticipation
}

bool SimNumbers::begin(unsigned long time)
{
    if(!common_begin(time, _fixed_freq))
        return false;
    
    // WPM is already stored from constructor, no need to update it
    
    // Start with interval signal phase
    _current_phase = PHASE_INTERVAL_SIGNAL;
    _interval_repeats_sent = 0;
    _groups_sent = 0;
    
    // Check if we have a valid realizer before accessing it
    if(_realizer == -1) {
        return false;
    }

    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    wavegen->set_frequency(NUMBERS_SPACE_FREQUENCY, false);    // Set _enabled and force frequency update with existing _vfo_freq
    // _vfo_freq should retain its value from the previous cycle
    _enabled = true;
    force_frequency_update();
    realize();  // CRITICAL: Set active state for audio output!
    
    generate_interval_signal();
    _morse.start_morse(_group_buffer, _wpm);  // No repeat, stations handle their own repetition

    return true;
}

void SimNumbers::realize()
{
    if(_realizer == -1) {
        return;  // No WaveGen allocated
    }
    
    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }
    
    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    wavegen->set_active_frequency(_active);
}

bool SimNumbers::update(Mode *mode)
{
    common_frequency_update(mode);
    
    if(_enabled && _realizer != -1){
        WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
        wavegen->set_frequency(_frequency);
    }

    realize();
    return true;
}

// JH! platformio memory inspection shows this method as one of the top 5 largest objects
bool SimNumbers::step(unsigned long time)
{    // Handle morse code timing
    int morse_state = _morse.step_morse(time);
      switch(morse_state){
        case STEP_MORSE_TURN_ON:
            _active = true;
            _transmission_active = true;
            realize();
            send_carrier_charge_pulse(_signal_meter);  // Send charge pulse when carrier turns on
            break;

        case STEP_MORSE_LEAVE_ON:
            // Carrier remains on - send another charge pulse
            send_carrier_charge_pulse(_signal_meter);
            break;

        case STEP_MORSE_TURN_OFF:
            _active = false;
            realize();
            // No charge pulse when carrier turns off
            break;
            
        case STEP_MORSE_LEAVE_OFF:
            _active = false;
            realize();
            // No charge pulse when carrier is off
            break;
              case STEP_MORSE_MESSAGE_COMPLETE:
            // Message transmission just completed!
            _active = false;
            _transmission_active = false;
            realize();
            
            // Handle phase transitions
            switch(_current_phase) {
                case PHASE_INTERVAL_SIGNAL:
                    _interval_repeats_sent++;
                    if(_interval_repeats_sent >= _total_interval_repeats) {
                        // Interval signal complete, start numbers phase
                        _current_phase = PHASE_NUMBERS;
                        _groups_sent = 0;
                        _next_group_time = time + INTER_GROUP_DELAY;  // Short delay before first number
                    } else {
                        // Send another "FT"
                        _next_group_time = time + INTER_GROUP_DELAY;
                    }
                    break;
                    
                case PHASE_NUMBERS:
                    _groups_sent++;
                    if(_groups_sent >= _total_groups_per_cycle) {
                        // Numbers complete, send ending sequence
                        _current_phase = PHASE_ENDING;
                        _next_group_time = time + INTER_GROUP_DELAY;
                    } else {
                        // More groups in cycle, short delay
                        _next_group_time = time + INTER_GROUP_DELAY;
                    }
                    break;
                      case PHASE_ENDING:
                    // Ending sequence complete, start cycle delay
                    _current_phase = PHASE_CYCLE_DELAY;
                    _next_group_time = time + INTER_CYCLE_DELAY;
                    
                    // DYNAMIC PIPELINING: Free WaveGen at end of complete cycle
                    // This allows other stations to use the WaveGen during our cycle delay
                    end();
                    break;
                    
                case PHASE_CYCLE_DELAY:
                    // This shouldn't happen (handled in delay check below)
                    break;
            }
            
            _in_inter_group_delay = true;
            break;}
      // Check if it's time for next transmission
    if(_in_inter_group_delay && time >= _next_group_time) {
        _in_inter_group_delay = false;
        
        switch(_current_phase) {            case PHASE_INTERVAL_SIGNAL:
                generate_interval_signal();
                _morse.start_morse(_group_buffer, _wpm);
                break;
                
            case PHASE_NUMBERS:
                generate_next_number_group();
                _morse.start_morse(_group_buffer, _wpm);
                break;
                
            case PHASE_ENDING:
                generate_ending_sequence();
                _morse.start_morse(_group_buffer, _wpm);
                break;            case PHASE_CYCLE_DELAY:
                // DYNAMIC PIPELINING: Try to reallocate WaveGen for next cycle
                if(begin(time)) {  // Only proceed if WaveGen is available
                    // Cycle complete, restart with interval signal
                    _current_phase = PHASE_INTERVAL_SIGNAL;
                    _interval_repeats_sent = 0;
                    _groups_sent = 0;                      // Add creepy frequency drift for new transmission cycle
                    apply_frequency_drift();
                    
                    // Immediately update the wave generator frequency to reflect the drift
                    // This ensures the audio frequency changes right away, not just when user tunes
                    force_frequency_update();
                    realize();  // CRITICAL: Reactivate after frequency drift update!
                      generate_interval_signal();
                    _morse.start_morse(_group_buffer, _wpm);
                } else {
                    // WaveGen not available - extend cycle delay and try again later
                    _next_group_time = time + 1000;  // Try again in 1 second
                }
                break;
        }
    }
    
    return true;
}
// JH! 

void SimNumbers::generate_next_number_group()
{
    // Generate one fresh random 5-digit group
    int digits[5];
    
    // Generate 5 random digits (0-9) - fresh every time!
    for(int i = 0; i < 5; i++) {
        digits[i] = random(10);
    }
    
    // Format as "XXXXX" (5 digits only, no space - we handle pauses with timing)
    sprintf(_group_buffer, "%d%d%d%d%d", 
            digits[0], digits[1], digits[2], digits[3], digits[4]);
}

void SimNumbers::generate_interval_signal()
{
    // Generate "FT" interval signal for FluxTune signature
    // This creates the anticipatory, hypnotic effect before numbers
    sprintf(_group_buffer, "FT");
}

void SimNumbers::generate_ending_sequence()
{
    // Generate "00000" ending sequence (standard numbers station ending)
    // This clearly marks the end of the message transmission
    sprintf(_group_buffer, "00000");
}

void SimNumbers::apply_frequency_drift()
{
    // Add slight frequency drift for authentic numbers station creepiness
    // Real numbers stations often drift slightly between transmissions
    // Drift range: ±200 Hz around the original frequency
    const float DRIFT_RANGE = 200.0f;
    
    // Use Arduino random() function
    float drift = ((float)random(0, (long)(2.0f * DRIFT_RANGE * 100))) / 100.0f - DRIFT_RANGE;
    
    // Apply drift to the base class frequency - the station will use this on next cycle
    _fixed_freq = _fixed_freq + drift;
}
