#include "sim_dual_transmitter.h"
#include "saved_data.h"  // For option_bfo_offset

SimDualTransmitter::SimDualTransmitter(WaveGenPool *wave_gen_pool, float fixed_freq) 
    : SimTransmitter(wave_gen_pool, fixed_freq)
{
    _realizer_b = -1;
}

bool SimDualTransmitter::begin(unsigned long time)
{
    // ATOMIC DUAL GENERATOR ACQUISITION
    // Both generators must be acquired successfully or entire operation fails
    
    // Step 1: Try to acquire first generator using base class
    if (!common_begin(time, _fixed_freq)) {
        return false;  // Failed to get first generator
    }
    
    // Step 2: Try to acquire second generator
    if (!acquire_second_generator()) {
        // CRITICAL: Release first generator since we failed to get both
        SimTransmitter::end();  // Release first generator
        return false;  // Failed to get second generator
    }
    
    // Step 3: Call derived class to initialize both generators
    bool success = begin_dual_generators(time);
    
    if (!success) {
        // Cleanup both generators on failure
        end();  // This will release both generators
        return false;
    }
    
    return true;
}

void SimDualTransmitter::end()
{
    // Release BOTH generators properly
    // This fixes the fundamental issue where base class end() only releases first generator
    
    // Release first generator using base class
    SimTransmitter::end();
    
    // Release second generator manually
    release_second_generator();
}

bool SimDualTransmitter::reinitialize(unsigned long time, float fixed_freq)
{
    // DUAL GENERATOR REINITIALIZE: Properly clean up both generators
    
    // Step 1: Clean up both generators using our dual-aware end() method
    end();  // This releases both generators properly
    
    // Step 2: Set new parameters (same as base class)
    _fixed_freq = fixed_freq;
    _frequency = 0.0;
    _enabled = false;
    _active = false;
    _station_state = ACTIVE;  // Station is now active at new frequency
    
    // Step 3: Start the station with new frequency (calls our dual-aware begin())
    bool success = begin(time);
    
    // Step 4: Force immediate frequency update for both generators
    if (success && _enabled) {
        force_wave_generator_refresh();
    }
    
    return success;
}

void SimDualTransmitter::force_wave_generator_refresh()
{
    // Update BOTH generators with current frequency
    // Base class force_wave_generator_refresh() only handles first generator
    
    if (_enabled && _realizer != -1 && _realizer_b != -1) {
        // Recalculate _frequency with current _fixed_freq and _vfo_freq
        float raw_frequency = _vfo_freq - _fixed_freq;
        _frequency = raw_frequency + option_bfo_offset;
        
        // Let derived class update both generators with new frequency
        realize_dual_generators();
    }
}

bool SimDualTransmitter::acquire_second_generator()
{
    if (_realizer_b != -1) {
        return true;  // Already have one
    }
    
    _realizer_b = _wave_gen_pool->get_realizer(_station_id);
    return (_realizer_b != -1);
}

void SimDualTransmitter::release_second_generator()
{
    if (_realizer_b != -1) {
        _wave_gen_pool->free_realizer(_realizer_b, _station_id);
        _realizer_b = -1;
    }
}

void SimDualTransmitter::silence_second_generator()
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
