#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_test.h"
#include "signal_meter.h"

SimTest::SimTest(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq,
                 float toggle_rate_hz, float tone_a_offset, float tone_b_offset) 
    : SimTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter),
      _toggle_rate_hz(toggle_rate_hz), _current_tone_a_offset(tone_a_offset), 
      _current_tone_b_offset(tone_b_offset)
{
    // Calculate toggle interval in milliseconds from Hz rate
    _toggle_interval = (unsigned long)(1000.0 / _toggle_rate_hz);
    
    // Ensure minimum interval of 10ms to prevent system overload
    if (_toggle_interval < 10) {
        _toggle_interval = 10;
        _toggle_rate_hz = 100.0; // Limit to 100 Hz maximum
    }
    
    // Test transmission will be started in begin() method
}

bool SimTest::begin(unsigned long time)
{
    if(!common_begin(time, _fixed_freq))
        return false;

    // Start test transmission with repeat enabled
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

void SimTest::realize()
{
    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }
    
    // Don't try to access wave generator if we don't have one
    if(_realizer == -1) {
        return;
    }
    
    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    
    if(_active) {
        // Use configurable frequencies with configurable toggle rate
        static bool toggle_state = false;
        static unsigned long last_realize_toggle = 0;
        unsigned long current_time = millis();
        
        if (current_time - last_realize_toggle > _toggle_interval) {
            toggle_state = !toggle_state;
            last_realize_toggle = current_time;
        }
        
        float tone_offset;
        if (toggle_state) {
            tone_offset = _current_tone_a_offset;  // Use configured tone A
        } else {
            tone_offset = _current_tone_b_offset;  // Use configured tone B
        }
        
        // Use the working SimPager method: _frequency + offset
        wavegen->set_frequency(_frequency + tone_offset, true);
        wavegen->set_frequency(_frequency + tone_offset, false);
    } else {
        // Explicitly set silent frequencies when inactive
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
    }
    
    wavegen->set_active_frequency(_active);
}

bool SimTest::update(Mode *mode)
{
    common_frequency_update(mode);

    if(_enabled) {
        // Note: We don't set frequencies here like RTTY does
        // Test frequencies are set in realize() based on current tone
    }

    realize();
    return true;
}

bool SimTest::step(unsigned long time)
{
    // Use configurable toggle rate for performance testing
    static unsigned long last_toggle = 0;
    static bool toggle_state = false;
    
    if (!_active) {
        // Turn on initially
        _active = true;
        realize();
        send_carrier_charge_pulse(_signal_meter);
    } else {
        // Toggle frequency at configured rate
        if (time - last_toggle > _toggle_interval) {
            toggle_state = !toggle_state;
            last_toggle = time;
            realize(); // This will pick up the new toggle_state in realize()
        }
        // Send charge pulse to keep signal meter active
        send_carrier_charge_pulse(_signal_meter);
    }

    return true;
}

void SimTest::generate_new_tone_pair()
{
    // Generate random tone pair similar to DTMF frequencies
    // Range: 650-1650 Hz offset, minimum 200 Hz separation
    
    float frequency_range = TEST_TONE_MAX_OFFSET - TEST_TONE_MIN_OFFSET;
    
    // Arduino random() function
    _current_tone_a_offset = TEST_TONE_MIN_OFFSET + 
        random((long)(frequency_range - TEST_TONE_MIN_SEPARATION));
    
    // Generate second tone with minimum separation
    float remaining_range = frequency_range - TEST_TONE_MIN_SEPARATION;
    float tone_b_base = random((long)remaining_range);
    
    // Ensure minimum separation
    if (tone_b_base < _current_tone_a_offset - TEST_TONE_MIN_OFFSET) {
        _current_tone_b_offset = TEST_TONE_MIN_OFFSET + tone_b_base;
    } else {
        _current_tone_b_offset = _current_tone_a_offset + TEST_TONE_MIN_SEPARATION + 
            (tone_b_base - (_current_tone_a_offset - TEST_TONE_MIN_OFFSET));
    }

    // Ensure tone B doesn't exceed maximum
    if (_current_tone_b_offset > TEST_TONE_MAX_OFFSET) {
        _current_tone_b_offset = TEST_TONE_MAX_OFFSET;
    }
}

void SimTest::debug_print_tone_pair() const
{
    // Debug output not needed for Arduino build
}
