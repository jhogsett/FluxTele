#ifndef __SIM_RING2_H__
#define __SIM_RING2_H__

#include "async_telco.h"
#include "sim_transmitter.h"

class SignalMeter; // Forward declaration

// North American telephone ring frequencies
#define RING2_TONE_LOW_OFFSET    440.0   // Generator 1: 440 Hz (standard telephone ring low tone)
#define RING2_TONE_HIGH_OFFSET   480.0   // Generator 2: 480 Hz (standard telephone ring high tone)

class SimRing2 : public SimTransmitter
{
public:
    SimRing2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);
    
    virtual bool begin(unsigned long time) override;
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;
    
    void realize();
    
    // Debug method to display current tone pair
    void debug_print_tone_pair() const;
    
    // Method to generate new tone pairs for testing
    void generate_new_tone_pair();

private:
    AsyncTelco _telco;
    float _current_tone_a_offset;
    float _current_tone_b_offset;
    SignalMeter *_signal_meter;     // Pointer to signal meter for charge pulses
};

#endif
