#ifndef __SIM_RING_H__
#define __SIM_RING_H__

// Step 6: DUAL GENERATOR MODE - The critical test (where it always fails)
//#define ENABLE_FIRST_GENERATOR  // Use first generator only (original behavior)
//#define ENABLE_SECOND_GENERATOR // Use second generator only (for testing)  
#define ENABLE_DUAL_GENERATOR   // Use both generators for dual-tone (target behavior)

#include "async_telco.h"
#include "sim_dual_transmitter.h"

class SignalMeter; // Forward declaration

// Ring tone frequency range (Hz offset from VFO) - North American telephone ring
#define RING_TONE_LOW_OFFSET 440.0     // First ring tone (440 Hz)
#define RING_TONE_HIGH_OFFSET 480.0    // Second ring tone (480 Hz) 
#define RING_TONE_MIN_SEPARATION 40.0  // Separation between 440 and 480 Hz tones

class SimRing : public SimDualTransmitter
{
public:
    SimRing(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);
    
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;
    
    void realize();  // Standard realize method expected by the system
    
    // Implement pure virtual methods from SimDualTransmitter
    virtual void realize_dual_generators() override;
    virtual bool begin_dual_generators(unsigned long time) override;
    
    // Debug method to display current tone pair
    void debug_print_tone_pair() const;
    
    // Method to generate new tone pairs for testing
    void generate_new_tone_pair();
    
    // Debug method to test dual generator acquisition
    void debug_test_dual_generator_acquisition();

private:
    AsyncTelco _telco;
    float _current_tone_a_offset;
    float _current_tone_b_offset;
    SignalMeter *_signal_meter;     // Pointer to signal meter for charge pulses
    
    // Legacy compatibility variables for second generator tone offsets
    float _current_tone_a_offset_b; // Second generator's tone A frequency offset
    float _current_tone_b_offset_b; // Second generator's tone B frequency offset
};

#endif
