#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_ring.h"
#include "signal_meter.h"

SimRingUnworking::SimRingUnworking(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq) 
    : SimTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter)
{
#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
    _realizer_b = -1;
#endif

    // Generate initial tone pair
    generate_new_tone_pair();
    // ring transmission will be started in begin() method
}

bool SimRingUnworking::begin(unsigned long time)
{
#ifdef ENABLE_FIRST_GENERATOR
    // FIRST GENERATOR ONLY MODE
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
#endif

#ifdef ENABLE_SECOND_GENERATOR
    // SECOND GENERATOR ONLY MODE
    if(!acquire_second_generator()) {
        return false;  // No second generator available
    }
    
    // Start ring transmission with repeat enabled
    _telco.start_telco_transmission(true);

    WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);

    // Initialize both channels to silent
    wavegen_b->set_frequency(SILENT_FREQ, false);
    wavegen_b->set_frequency(SILENT_FREQ, true);
#endif

#ifdef ENABLE_DUAL_GENERATOR
    // DUAL GENERATOR MODE: ATOMIC ACQUISITION - both must succeed or entire operation fails
    
    // Step 1: Try to acquire first generator
    if(!common_begin(time, _fixed_freq)) {
        Serial.println("DUAL MODE ERROR: Failed to acquire first generator");
        return false;  // Failed to get first generator
    }
    
    // Step 2: Try to acquire second generator
    if(!acquire_second_generator()) {
        Serial.println("DUAL MODE ERROR: Failed to acquire second generator, releasing first");
        // CRITICAL: Release first generator since we failed to get both
        end();  // This releases the first generator
        return false;  // Failed to get second generator
    }
    
    Serial.println("DUAL MODE SUCCESS: Both generators acquired");
    Serial.print("First generator realizer: ");
    Serial.println(_realizer);
    Serial.print("Second generator realizer: ");
    Serial.println(_realizer_b);
    
    // DEBUG: Show the ring frequencies that will be used
    Serial.print("Ring Tone A frequency offset: ");
    Serial.print(_current_tone_a_offset);
    Serial.println(" Hz");
    Serial.print("Ring Tone B frequency offset: ");
    Serial.print(_current_tone_b_offset);
    Serial.println(" Hz");
    Serial.print("Base station frequency: ");
    Serial.print(_fixed_freq);
    Serial.println(" Hz");
    Serial.print("Final Ring Tone A frequency: ");
    Serial.print(_fixed_freq + _current_tone_a_offset);
    Serial.println(" Hz");
    Serial.print("Final Ring Tone B frequency: ");
    Serial.print(_fixed_freq + _current_tone_b_offset);
    Serial.println(" Hz");
    
    // Start ring transmission with repeat enabled
    _telco.start_telco_transmission(true);

    // Initialize both generators to silent
    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);

    // Initialize first generator channels to silent
    wavegen->set_frequency(SILENT_FREQ, false);
    wavegen->set_frequency(SILENT_FREQ, true);
    
    // Initialize second generator channels to silent
    wavegen_b->set_frequency(SILENT_FREQ, false);
    wavegen_b->set_frequency(SILENT_FREQ, true);
#endif

    return true;
}

void SimRingUnworking::realize()
{
    Serial.print("REALIZE DEBUG: _frequency = ");
    Serial.println(_frequency);
    Serial.print("REALIZE DEBUG: _fixed_freq = ");
    Serial.println(_fixed_freq);
    Serial.print("REALIZE DEBUG: tone offsets = ");
    Serial.print(_current_tone_a_offset);
    Serial.print(" / ");
    Serial.println(_current_tone_b_offset);
    
    if(!check_frequency_bounds()) {
        Serial.println("REALIZE DEBUG: check_frequency_bounds() FAILED - exiting early");
        Serial.print("REALIZE DEBUG: Computed frequency was: ");
        Serial.println(_frequency);
        return;  // Out of audible range
    }
    
    Serial.println("REALIZE DEBUG: check_frequency_bounds() PASSED - continuing");
    
#ifdef ENABLE_FIRST_GENERATOR
    // FIRST GENERATOR ONLY MODE
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
        }    } else {
        // Explicitly set silent frequencies when inactive (during SILENCE)
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
    }
    
    wavegen->set_active_frequency(_active);
#endif

#ifdef ENABLE_SECOND_GENERATOR
    // SECOND GENERATOR ONLY MODE
    // Use second generator with its own tone offsets
    if(_realizer_b == -1) {
        return;
    }
    
    WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
    
    if(_active) {
        // Set frequencies based on current ring state - using second generator tone offsets
        switch(_telco.get_current_state()) {
            case TELCO_STATE_TONE_A:
                // Transmit Tone A (longer unsquelch tone) - second generator frequencies
                wavegen_b->set_frequency(_frequency + _current_tone_a_offset_b, true);
                wavegen_b->set_frequency(_frequency + _current_tone_a_offset_b, false);
                break;
                
            case TELCO_STATE_TONE_B:
                // Transmit Tone B (shorter identification tone) - second generator frequencies
                wavegen_b->set_frequency(_frequency + _current_tone_b_offset_b, true);
                wavegen_b->set_frequency(_frequency + _current_tone_b_offset_b, false);
                break;
                  default:
                // Silent state (SILENCE) - should not reach here when _active is true
                wavegen_b->set_frequency(SILENT_FREQ, true);
                wavegen_b->set_frequency(SILENT_FREQ, false);
                break;
        }    } else {
        // Explicitly set silent frequencies when inactive (during SILENCE)
        wavegen_b->set_frequency(SILENT_FREQ, true);
        wavegen_b->set_frequency(SILENT_FREQ, false);
    }
    
    wavegen_b->set_active_frequency(_active);
#endif

#ifdef ENABLE_DUAL_GENERATOR
    // DUAL GENERATOR MODE: Control both generators simultaneously
    // Ensure we have both generators before proceeding
    if(_realizer == -1 || _realizer_b == -1) {
        Serial.println("DUAL MODE ERROR: Missing generator in realize()");
        return;
    }
    
    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
    
    if(_active) {
        // Set frequencies for BOTH generators based on current ring state
        switch(_telco.get_current_state()) {
            case TELCO_STATE_TONE_A:
                // TELEPHONE RING: Proper frequency calculation (VFO + BFO + tone offset)
                float freq1 = _frequency + _current_tone_a_offset;  // Should be BFO + 440 Hz
                Serial.print("Gen1 calculated: ");
                Serial.print(_frequency);
                Serial.print(" + ");
                Serial.print(_current_tone_a_offset);
                Serial.print(" = ");
                Serial.println(freq1);
                wavegen->set_frequency(freq1, true);
                wavegen->set_frequency(freq1, false);
                
                // SECOND GENERATOR: Proper frequency calculation  
                float freq2 = _frequency + _current_tone_b_offset;  // Should be BFO + 480 Hz
                Serial.print("Gen2 calculated: ");
                Serial.print(_frequency);
                Serial.print(" + ");
                Serial.print(_current_tone_b_offset);
                Serial.print(" = ");
                Serial.println(freq2);
                wavegen_b->set_frequency(freq2, true);
                wavegen_b->set_frequency(freq2, false);
                break;
                
            case TELCO_STATE_TONE_B:
                // FIRST GENERATOR: Transmit Tone B (should not happen in ring mode)
                wavegen->set_frequency(_frequency + _current_tone_b_offset, true);
                wavegen->set_frequency(_frequency + _current_tone_b_offset, false);
                
                // SECOND GENERATOR: Also Tone B (should not happen in ring mode)
                wavegen_b->set_frequency(_frequency + _current_tone_b_offset, true);
                wavegen_b->set_frequency(_frequency + _current_tone_b_offset, false);
                break;
                  default:
                // Silent state for both generators
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                wavegen_b->set_frequency(SILENT_FREQ, true);
                wavegen_b->set_frequency(SILENT_FREQ, false);
                break;
        }    } else {
        // Both generators silent when inactive
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
        wavegen_b->set_frequency(SILENT_FREQ, true);
        wavegen_b->set_frequency(SILENT_FREQ, false);
    }
    
    // Activate/deactivate both generators together
    wavegen->set_active_frequency(_active);
    wavegen_b->set_active_frequency(_active);
#endif
}

bool SimRingUnworking::update(Mode *mode)
{
    common_frequency_update(mode);

    if(_enabled) {
        // Note: We don't set frequencies here like RTTY does
        // ring frequencies are set in realize() based on current tone
    }

    realize();
    return true;
}

bool SimRingUnworking::step(unsigned long time)
{
    switch(_telco.step_telco(time)) {        case STEP_TELCO_TURN_ON:
            // Check if this is the start of a new page cycle (silence → tone A)
            if (_telco.get_current_state() == TELCO_STATE_TONE_A) {
                generate_new_tone_pair();
                
#ifdef ENABLE_FIRST_GENERATOR
                // FIRST GENERATOR ONLY: Acquire wave generator after silent period
                if(_realizer == -1) {
                    if(!common_begin(time, _fixed_freq)) {
                        // Failed to get wave generator - stay inactive
                        _active = false;
                        return true;
                    }
                    // CRITICAL: Force frequency update after reacquiring generator
                    force_frequency_update();
                }
#endif

#ifdef ENABLE_SECOND_GENERATOR
                // SECOND GENERATOR ONLY: Acquire second generator after silent period
                if(_realizer_b == -1) {
                    if(!acquire_second_generator()) {
                        // Failed to get second generator - stay inactive
                        _active = false;
                        return true;
                    }
                    // CRITICAL: Force frequency update after reacquiring generator
                    force_frequency_update();
                }
#endif

#ifdef ENABLE_DUAL_GENERATOR
                // DUAL GENERATOR MODE: ATOMIC REACQUISITION - both must succeed
                bool need_first = (_realizer == -1);
                bool need_second = (_realizer_b == -1);
                
                if (need_first || need_second) {
                    Serial.println("DUAL MODE: Reacquiring generators after silence");
                    
                    // Try to acquire first generator if needed
                    if (need_first && !common_begin(time, _fixed_freq)) {
                        Serial.println("DUAL MODE ERROR: Failed to reacquire first generator");
                        _active = false;
                        return true;
                    }
                    
                    // Try to acquire second generator if needed
                    if (need_second && !acquire_second_generator()) {
                        Serial.println("DUAL MODE ERROR: Failed to reacquire second generator, releasing first");
                        // Release first generator if we just acquired it
                        if (need_first) {
                            end();
                        }
                        _active = false;
                        return true;
                    }
                    
                    Serial.println("DUAL MODE SUCCESS: Both generators reacquired");
                    // CRITICAL: Force frequency update after reacquiring generators
                    force_frequency_update();
                }
#endif
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
            
#ifdef ENABLE_FIRST_GENERATOR
            // FIRST GENERATOR ONLY: Release wave generator during silent period
            if(_realizer != -1) {
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                wavegen->set_active_frequency(false);
            }
            // Release the resource for other stations to use during silence
            end();  // This calls Realization::end() which frees the realizer
#endif

#ifdef ENABLE_SECOND_GENERATOR
            // SECOND GENERATOR ONLY: Release second generator during silent period
            silence_second_generator();
            release_second_generator();
#endif

#ifdef ENABLE_DUAL_GENERATOR
            // DUAL GENERATOR MODE: Release both generators during silent period
            Serial.println("DUAL MODE: Releasing both generators during silence");
            
            // Silence both generators first
            if(_realizer != -1) {
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                wavegen->set_active_frequency(false);
            }
            
            silence_second_generator();
            
            // Release both generators
            end();  // Release first generator
            release_second_generator();  // Release second generator
#endif
            
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

void SimRingUnworking::generate_new_tone_pair()
{
    // Fixed North American telephone ring frequencies
    // For dual-tone ring: Generator 1 = 440 Hz, Generator 2 = 480 Hz
    
    _current_tone_a_offset = RING_TONE_LOW_OFFSET;   // Generator 1: 440 Hz
    _current_tone_b_offset = RING_TONE_HIGH_OFFSET;  // Generator 2: 480 Hz

#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
    // For compatibility - not currently used in dual generator mode
    _current_tone_a_offset_b = RING_TONE_HIGH_OFFSET;  // 480 Hz 
    _current_tone_b_offset_b = 0;  // Unused
#endif
}

void SimRingUnworking::debug_print_tone_pair() const
{
    // Debug output not needed for Arduino build
}

void SimRingUnworking::debug_test_dual_generator_acquisition()
{
    // DEBUG: Test if we can acquire both generators simultaneously
    Serial.println("=== DUAL GENERATOR ACQUISITION TEST ===");
    
    // Show current state
    Serial.print("Current first generator (realizer): ");
    Serial.println(_realizer);
    
#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
    Serial.print("Current second generator (realizer_b): ");
    Serial.println(_realizer_b);
#endif
    
    // Test: Try to acquire first generator if we don't have it
    bool first_acquired = false;
    int original_first_realizer = _realizer;
    
    if (_realizer == -1) {
        Serial.println("Attempting to acquire FIRST generator...");
        if (common_begin(millis(), _fixed_freq)) {
            Serial.print("SUCCESS: First generator acquired, realizer = ");
            Serial.println(_realizer);
            first_acquired = true;
        } else {
            Serial.println("FAILED: Could not acquire first generator");
        }
    } else {
        Serial.println("First generator already acquired");
        first_acquired = true;
    }
    
#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
    // Test: Try to acquire second generator if we don't have it
    bool second_acquired = false;
    int original_second_realizer = _realizer_b;
    
    if (_realizer_b == -1) {
        Serial.println("Attempting to acquire SECOND generator...");
        if (acquire_second_generator()) {
            Serial.print("SUCCESS: Second generator acquired, realizer_b = ");
            Serial.println(_realizer_b);
            second_acquired = true;
        } else {
            Serial.println("FAILED: Could not acquire second generator");
        }
    } else {
        Serial.println("Second generator already acquired");
        second_acquired = true;
    }
    
    // Report results
    Serial.print("RESULT: First=");
    Serial.print(first_acquired ? "OK" : "FAIL");
    Serial.print(", Second=");
    Serial.print(second_acquired ? "OK" : "FAIL");
    Serial.print(", Both=");
    Serial.println((first_acquired && second_acquired) ? "SUCCESS" : "FAILED");
    
    // Show wave generator pool status
    Serial.println("Wave generator pool status:");
    for (int i = 0; i < 4; i++) {
        // We can't directly access pool internals, so just show our realizers
        if (i == _realizer) {
            Serial.print("  Generator ");
            Serial.print(i);
            Serial.println(": USED (first)");
        } else if (i == _realizer_b) {
            Serial.print("  Generator ");
            Serial.print(i);
            Serial.println(": USED (second)");
        }
    }
    
    // Cleanup: Release any generators we acquired during test
    if (first_acquired && original_first_realizer == -1) {
        Serial.println("Releasing first generator (acquired during test)");
        end(); // This releases the first generator
    }
    
    if (second_acquired && original_second_realizer == -1) {
        Serial.println("Releasing second generator (acquired during test)");
        release_second_generator();
    }
#else
    Serial.println("Second generator support not compiled in");
#endif
    
    Serial.println("=== END DUAL GENERATOR TEST ===");
}

void SimRingUnworking::end()
{
    // Call parent class end method
    SimTransmitter::end();
}

// SECOND GENERATOR HELPER METHODS
#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
bool SimRingUnworking::acquire_second_generator()
{
    if (_realizer_b != -1) {
        return true;  // Already have one
    }
    
    _realizer_b = _wave_gen_pool->get_realizer(_station_id);
    return (_realizer_b != -1);
}

void SimRingUnworking::release_second_generator()
{
    if (_realizer_b != -1) {
        _wave_gen_pool->free_realizer(_realizer_b, _station_id);
        _realizer_b = -1;
    }
}

void SimRingUnworking::silence_second_generator()
{
    if (_realizer_b != -1) {
        WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
        if (wavegen_b) {
            wavegen_b->set_frequency(SILENT_FREQ, true);
            wavegen_b->set_frequency(SILENT_FREQ, false);
            wavegen_b->set_active_frequency(false);
        }
    }
}
#endif
