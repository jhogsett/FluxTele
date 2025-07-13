#ifndef __SIM_PAGER2_H__
#define __SIM_PAGER2_H__

// Step 6: DUAL GENERATOR MODE - The critical test (where it always fails)
//#define ENABLE_FIRST_GENERATOR  // Use first generator only (original behavior)
//#define ENABLE_SECOND_GENERATOR // Use second generator only (for testing)  
#define ENABLE_DUAL_GENERATOR   // Use both generators for dual-tone (target behavior)

#include "async_pager.h"
#include "sim_transmitter.h"

class SignalMeter; // Forward declaration

// Pager tone frequency range (Hz offset from VFO) - DTMF-like range for pleasant listening
#define PAGER2_TONE_MIN_OFFSET 650.0    // Minimum tone frequency offset (DTMF-like range)
#define PAGER2_TONE_MAX_OFFSET 1650.0   // Maximum tone frequency offset (DTMF-like range)
#define PAGER2_TONE_MIN_SEPARATION 100.0 // Minimum separation between tones (suitable for DTMF-like range)

class SimPager2 : public SimTransmitter
{
public:
    SimPager2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);
    
    virtual bool begin(unsigned long time) override;
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;
    virtual void end() override;
    
    void realize();
    
    // Debug method to display current tone pair
    void debug_print_tone_pair() const;
    
    // Method to generate new tone pairs for testing
    void generate_new_tone_pair();
    
    // Debug method to test dual generator acquisition
    void debug_test_dual_generator_acquisition();

private:
    AsyncPager _pager;
    float _current_tone_a_offset;
    float _current_tone_b_offset;
    SignalMeter *_signal_meter;     // Pointer to signal meter for charge pulses
    
#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
    // Second generator support - separate wave generator for testing/dual-tone
    int _realizer_b;                // Second wave generator realizer ID
    float _current_tone_a_offset_b; // Second generator's tone A frequency offset
    float _current_tone_b_offset_b; // Second generator's tone B frequency offset
    
    // Helper methods for second generator management
    bool acquire_second_generator();
    void release_second_generator();
    void silence_second_generator();
#endif
};

#endif
