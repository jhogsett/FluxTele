#include "sim_ring_dual.h"
#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "signal_meter.h"

// Temporary debug output for resource testing - disable for production to save memory
// #define DEBUG_RING_RESOURCES

SimRing::SimRing(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq)
    : SimDualTone(wave_gen_pool, fixed_freq, RING_TONE_LOW_OFFSET, RING_TONE_HIGH_OFFSET), 
      _signal_meter(signal_meter)
{
    // Initialize ring state machine
    _ring_state = RING_ACQUIRING;
    _state_start_time = 0;
    _next_retry_time = 0;

    // Initialize timing with slight randomization for realism
    randomize_timing();
}

bool SimRing::begin(unsigned long time)
{
    // Attempt to acquire all required realizers atomically
    if(!common_begin(time, _fixed_freq)) {
        // Failed to acquire wave generators - enter retry state
        enter_retry_state(time);
        return true;  // Return true to keep station alive for retry
    }

    // Initialize all acquired wave generators to silent
    stop_ring_tone();

    // Set enabled and force frequency update
    _enabled = true;
    force_frequency_update();
    realize();  // Set active state for audio output

    // Enter acquiring state to start the ring cycle
    enter_acquiring_state(time);

#ifdef DEBUG_RING_RESOURCES
    Serial.print(F("[SimRing] Started at "));
    Serial.print(_fixed_freq / 1000.0);
    Serial.println(F(" kHz"));
#endif

    return true;
}

bool SimRing::update(Mode *mode)
{
    if(!_enabled) {
        return false;
    }

    // Calculate frequency based on current VFO position
    common_frequency_update(mode);

    // Check if frequency is still in audible range
    if(!check_frequency_bounds()) {
        return false;
    }

    // Update wave generators with new frequencies
    force_frequency_update();

    return true;
}

bool SimRing::step(unsigned long time)
{
    if(!_enabled) {
        return false;
    }

    // Ring cadence state machine
    switch(_ring_state) {
        case RING_ACQUIRING:
            // We should already have generators if we reach this state
            // Start playing ring tone immediately
            enter_playing_state(time);
            break;

        case RING_PLAYING:
            // Check if ring tone duration has elapsed
            if(time >= _state_start_time + _ring_on_duration) {
                enter_silent_state(time);
            } else {
                // Continue playing ring tone and send charge pulses
                send_carrier_charge_pulse(_signal_meter);
            }
            break;

        case RING_SILENT:
            // Check if silent duration has elapsed
            if(time >= _state_start_time + _ring_off_duration) {
                // Try to start next ring cycle
                enter_playing_state(time);
            }
            // No charge pulses during silent period
            break;

        case RING_RETRY:
            // Check if it's time to retry generator acquisition
            if(time >= _next_retry_time) {
                // Try to re-acquire generators
                if(common_begin(time, _fixed_freq)) {
                    // Success! Initialize and start ringing
                    stop_ring_tone();
                    _enabled = true;
                    force_frequency_update();
                    realize();
                    enter_playing_state(time);
                } else {
                    // Still failed - schedule another retry
                    enter_retry_state(time);
                }
            }
            break;
    }

    return true;
}

void SimRing::end()
{
    _enabled = false;
    _active = false;
    
    // Stop any ring tone
    stop_ring_tone();
    
    // Release all acquired realizers
    SimDualTone::end();
}

void SimRing::realize()
{
    setActive(true);
}

void SimRing::randomize()
{
    // Add slight variations to ring timing for realism
    randomize_timing();
}

void SimRing::set_retry_state(unsigned long next_try_time)
{
    _next_retry_time = next_try_time;
    _ring_state = RING_RETRY;
}

// State machine methods
void SimRing::enter_acquiring_state(unsigned long time)
{
    _ring_state = RING_ACQUIRING;
    _state_start_time = time;
    
#ifdef DEBUG_RING_RESOURCES
    Serial.println(F("[SimRing] Entering ACQUIRING state"));
#endif
}

void SimRing::enter_playing_state(unsigned long time)
{
    _ring_state = RING_PLAYING;
    _state_start_time = time;
    
    // Start playing ring tone
    start_ring_tone();
    
#ifdef DEBUG_RING_RESOURCES
    Serial.print(F("[SimRing] Entering PLAYING state for "));
    Serial.print(_ring_on_duration);
    Serial.println(F(" ms"));
#endif
}

void SimRing::enter_silent_state(unsigned long time)
{
    _ring_state = RING_SILENT;
    _state_start_time = time;
    
    // Stop ring tone
    stop_ring_tone();
    
    // Add slight timing variation for next cycle
    randomize_timing();
    
#ifdef DEBUG_RING_RESOURCES
    Serial.print(F("[SimRing] Entering SILENT state for "));
    Serial.print(_ring_off_duration);
    Serial.println(F(" ms"));
#endif
}

void SimRing::enter_retry_state(unsigned long time)
{
    _ring_state = RING_RETRY;
    _next_retry_time = time + RING_RETRY_DELAY_MS;
    
    // Make sure we're not outputting any audio during retry
    stop_ring_tone();
    
#ifdef DEBUG_RING_RESOURCES
    Serial.println(F("[SimRing] Entering RETRY state"));
#endif
}

// Utility methods
void SimRing::start_ring_tone()
{
    if(!_enabled) return;
    
    // Update frequencies based on current VFO and start both generators
#if defined(ENABLE_GENERATOR_A) && defined(ENABLE_GENERATOR_C)
    // Dual generator mode: first generator gets 440 Hz tone, second gets 480 Hz tone
    if(get_realizer_count() >= 2) {
        int realizer_a = get_realizer(0);
        int realizer_c = get_realizer(1);
        
        if(realizer_a != -1) {
            WaveGen *wavegen_a = _wave_gen_pool->access_realizer(realizer_a);
            wavegen_a->set_frequency(_frequency);  // 440 Hz tone
        }
        
        if(realizer_c != -1) {
            WaveGen *wavegen_c = _wave_gen_pool->access_realizer(realizer_c);
            wavegen_c->set_frequency(_frequency_c);  // 480 Hz tone
        }
    }
#elif defined(ENABLE_GENERATOR_A)
    // Single generator mode: use 440 Hz tone only
    if(get_realizer_count() >= 1) {
        int realizer = get_realizer(0);
        if(realizer != -1) {
            WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
            wavegen->set_frequency(_frequency);  // 440 Hz tone
        }
    }
#elif defined(ENABLE_GENERATOR_C)
    // Single generator mode: use 440 Hz tone only
    if(get_realizer_count() >= 1) {
        int realizer = get_realizer(0);
        if(realizer != -1) {
            WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
            wavegen->set_frequency(_frequency_c);  // 440 Hz tone
        }
    }
#endif
}

void SimRing::stop_ring_tone()
{
    if(!_enabled) return;
    
    // Set all generators to silent frequency
    for(int i = 0; i < get_realizer_count(); i++) {
        int realizer = get_realizer(i);
        if(realizer != -1) {
            WaveGen *wavegen = _wave_gen_pool->access_realizer(realizer);
            wavegen->set_frequency(SILENT_FREQ_DT);
        }
    }
}

void SimRing::randomize_timing()
{
    // Add ±5% variation to ring timing for realism
    int variation_on = (RING_ON_DURATION_MS * (random(11) - 5)) / 100;  // ±5%
    int variation_off = (RING_OFF_DURATION_MS * (random(11) - 5)) / 100;  // ±5%
    
    _ring_on_duration = RING_ON_DURATION_MS + variation_on;
    _ring_off_duration = RING_OFF_DURATION_MS + variation_off;
    
    // Ensure minimum/maximum bounds
    if(_ring_on_duration < 1500) _ring_on_duration = 1500;   // Min 1.5s
    if(_ring_on_duration > 2500) _ring_on_duration = 2500;   // Max 2.5s
    if(_ring_off_duration < 3500) _ring_off_duration = 3500; // Min 3.5s
    if(_ring_off_duration > 4500) _ring_off_duration = 4500; // Max 4.5s
}
