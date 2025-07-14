#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_ring.h"
#include "signal_meter.h"
#include "saved_data.h"  // For option_bfo_offset

SimRing::SimRing(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq) 
    : SimDualTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter)
{
    // Generate initial tone pair
    generate_new_tone_pair();
    // ring transmission will be started in begin() method
}

bool SimRing::begin_dual_generators(unsigned long time)
{
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
    
    return true;
}

void SimRing::realize_dual_generators()
{
    if (!check_frequency_bounds()) {
        return;  // Out of audible range
    }
    
    // Ensure we have both generators before proceeding
    if (_realizer == -1 || _realizer_b == -1) {
        Serial.print("SimRing::realize_dual_generators() - Missing generators: _realizer=");
        Serial.print(_realizer);
        Serial.print(", _realizer_b=");
        Serial.println(_realizer_b);
        return;
    }
    
    // DEBUG: Track what frequencies we're actually setting
    Serial.print("SimRing::realize_dual_generators() - _frequency=");
    Serial.print(_frequency);
    Serial.print(", tone_a_offset=");
    Serial.print(_current_tone_a_offset);
    Serial.print(", tone_b_offset=");
    Serial.print(_current_tone_b_offset);
    Serial.print(", _active=");
    Serial.println(_active);
    
    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
    
    if (_active) {
        // Set frequencies for BOTH generators based on current ring state
        switch (_telco.get_current_state()) {
            case TELCO_STATE_TONE_A:
                // TELEPHONE RING: Generator 1 = 440 Hz, Generator 2 = 480 Hz
                float freq1 = _frequency + _current_tone_a_offset;
                float freq2 = _frequency + _current_tone_b_offset;
                
                Serial.print("Setting Gen1=");
                Serial.print(freq1);
                Serial.print(", Gen2=");
                Serial.println(freq2);
                
                wavegen->set_frequency(freq1, true);
                wavegen->set_frequency(freq1, false);
                
                wavegen_b->set_frequency(freq2, true);
                wavegen_b->set_frequency(freq2, false);
                break;
                
            case TELCO_STATE_TONE_B:
                // Ring mode should not use TONE_B, but handle it gracefully
                wavegen->set_frequency(_frequency + _current_tone_b_offset, true);
                wavegen->set_frequency(_frequency + _current_tone_b_offset, false);
                
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
        }
    } else {
        // Both generators silent when inactive
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
        wavegen_b->set_frequency(SILENT_FREQ, true);
        wavegen_b->set_frequency(SILENT_FREQ, false);
    }
    
    // Activate/deactivate both generators together
    wavegen->set_active_frequency(_active);
    wavegen_b->set_active_frequency(_active);
}

bool SimRing::update(Mode *mode)
{
    // CRITICAL: This method updates _frequency based on VFO tuning
    // Without this, the station ignores tuning knob changes
    common_frequency_update(mode);

    // DEBUG: Track frequency variables during tuning
    Serial.print("SimRing::update() - _fixed_freq=");
    Serial.print(_fixed_freq);
    Serial.print(", _vfo_freq=");
    Serial.print(_vfo_freq);
    Serial.print(", _frequency=");
    Serial.print(_frequency);
    Serial.print(", _enabled=");
    Serial.println(_enabled);

    if (_enabled) {
        // Ring frequencies are set in realize() based on current tone
        // No additional frequency setting needed here like RTTY does
    }

    // Update both generators with current frequency
    realize();  // Use standard realize() method like other stations
    return true;
}

void SimRing::realize()
{
    // Standard realize method that delegates to our dual-generator implementation
    // This maintains compatibility with the existing station management system
    realize_dual_generators();
}

bool SimRing::step(unsigned long time)
{
    switch(_telco.step_telco(time)) {
        case STEP_TELCO_TURN_ON:
            // Check if this is the start of a new page cycle (silence → tone A)
            if (_telco.get_current_state() == TELCO_STATE_TONE_A) {
                generate_new_tone_pair();
                
                // DUAL GENERATOR MODE: ATOMIC REACQUISITION handled by base class
                bool need_first = (_realizer == -1);
                bool need_second = (_realizer_b == -1);
                
                if (need_first || need_second) {
                    // Try to reacquire generators through base class begin() method
                    if (!begin(time)) {
                        _active = false;
                        return true;
                    }
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
            
            // DUAL GENERATOR MODE: Release both generators during silent period
            // Silence both generators first
            if (_realizer != -1) {
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                wavegen->set_active_frequency(false);
            }
            
            silence_second_generator();
            
            // Release both generators using base class end() method
            end();  // This properly releases both generators
            
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

void SimRing::generate_new_tone_pair()
{
    // Fixed North American telephone ring frequencies
    // For dual-tone ring: Generator 1 = 440 Hz, Generator 2 = 480 Hz
    
    _current_tone_a_offset = RING_TONE_LOW_OFFSET;   // Generator 1: 440 Hz
    _current_tone_b_offset = RING_TONE_HIGH_OFFSET;  // Generator 2: 480 Hz

    // For compatibility - not currently used in dual generator mode
    _current_tone_a_offset_b = RING_TONE_HIGH_OFFSET;  // 480 Hz 
    _current_tone_b_offset_b = 0;  // Unused
}

void SimRing::debug_print_tone_pair() const
{
    // Debug output not needed for Arduino build
}

void SimRing::debug_test_dual_generator_acquisition()
{
    // DEBUG: Test if we can acquire both generators simultaneously
    Serial.println("=== DUAL GENERATOR ACQUISITION TEST ===");
    
    // Show current state
    Serial.print("Current first generator (realizer): ");
    Serial.println(_realizer);
    
    Serial.print("Current second generator (realizer_b): ");
    Serial.println(_realizer_b);
    
    Serial.println("=== END DUAL GENERATOR TEST ===");
}
