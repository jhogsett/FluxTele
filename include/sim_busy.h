#ifndef __SIM_BUSY_H__
#define __SIM_BUSY_H__

#include "async_telco.h"
#include "sim_transmitter.h"

class SignalMeter; // Forward declaration

// Standard North American busy signal frequencies (480 Hz + 620 Hz)
// These are the fixed offsets that will be added to the station frequency
#define BUSY_TONE_LOW_OFFSET  480.0   // 480 Hz busy signal frequency offset
#define BUSY_TONE_HIGH_OFFSET 620.0   // 620 Hz busy signal frequency offset

class SimBusy : public SimTransmitter
{
public:
    SimBusy(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);
    
    virtual bool begin(unsigned long time) override;
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;
    virtual void end() override;
    
    void realize();
    
    // Debug method to display current tone pair
    void debug_print_tone_pair() const;

private:
    AsyncTelco _telco;
    SignalMeter *_signal_meter;     // Pointer to signal meter for charge pulses
    
    // Dual generator support for simultaneous 480 Hz + 620 Hz busy signal
    int _realizer_b;                // Second wave generator realizer ID
    
    // Helper methods
    bool acquire_second_generator();
    void release_wave_generators_during_silence();
};

#endif
