#ifndef __SIM_PAGER_H__
#define __SIM_PAGER_H__

#include "async_pager.h"
#include "sim_transmitter.h"

class SignalMeter; // Forward declaration

// Pager tone frequency range (Hz offset from VFO) - DTMF-like range for pleasant listening
#define PAGER_TONE_MIN_OFFSET 650.0    // Minimum tone frequency offset (DTMF-like range)
#define PAGER_TONE_MAX_OFFSET 1650.0   // Maximum tone frequency offset (DTMF-like range)
#define PAGER_TONE_MIN_SEPARATION 100.0 // Minimum separation between tones (suitable for DTMF-like range)

class SimPager : public SimTransmitter
{
public:
    SimPager(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);
      virtual bool begin(unsigned long time) override;
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;
      void realize();
      // Debug method to display current tone pair
    void debug_print_tone_pair() const;
    
    // Method to generate new tone pairs for testing
    void generate_new_tone_pair();

private:
    AsyncPager _pager;
    float _current_tone_a_offset;
    float _current_tone_b_offset;
    SignalMeter *_signal_meter;     // Pointer to signal meter for charge pulses
};

#endif
