#ifndef __SIM_DUAL_TRANSMITTER_H__
#define __SIM_DUAL_TRANSMITTER_H__

#include "sim_transmitter.h"

/**
 * Base class for dual-generator simulated transmitters.
 * Extends SimTransmitter to properly handle two AD9833 wave generators.
 * 
 * This class solves the fundamental issue where the base SimTransmitter class
 * was designed for single generators and doesn't properly handle dual-generator
 * cleanup during station relocation.
 *
 * DUAL GENERATOR ARCHITECTURE:
 * - _realizer: First generator (inherited from SimTransmitter)
 * - _realizer_b: Second generator (added by this class)
 * - Both generators managed atomically (acquire both or neither)
 * - Both generators released together during relocation
 * - Both generators updated together during frequency changes
 */
class SimDualTransmitter : public SimTransmitter
{
public:
    SimDualTransmitter(WaveGenPool *wave_gen_pool, float fixed_freq = 0.0);
    
    // Override base class methods to handle dual generators
    virtual bool begin(unsigned long time) override;
    virtual void end() override;
    virtual bool reinitialize(unsigned long time, float fixed_freq) override;
    virtual void force_wave_generator_refresh() override;

protected:
    // Dual generator management methods
    bool acquire_second_generator();
    void release_second_generator();
    void silence_second_generator();
    
    // Pure virtual methods for dual generator control
    // Derived classes must implement these to handle their specific dual-tone logic
    virtual void realize_dual_generators() = 0;  // Set frequencies on both generators
    virtual bool begin_dual_generators(unsigned long time) = 0;  // Initialize both generators
    
    // Second generator realizer ID
    int _realizer_b;
};

#endif
