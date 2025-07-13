#ifndef __VFO_H__
#define __VFO_H__

#include "mode.h"
#include "realization_pool.h"

// Forward declaration
class SignalMeter;

// need to define a set of bands

class VFO : public Mode
{
public:
    // constructor
    VFO(const char *title, float frequency, unsigned long step, RealizationPool *realization_pool);
    
    virtual void update_display(HT16K33Disp *display);
    virtual void update_signal_meter(SignalMeter *signal_meter);
    virtual void update_realization();
    void force_transmitter_refresh();  // Force hardware refresh when switching to SimRadio

    // JH! I am uncomfortable with the term "dirty", terms like "needing refresh" or "invalidate" would be better 
    void mark_hardware_dirty();  // Mark hardware as needing refresh

    // Static utility for stations to calculate signal strength charge based on VFO proximity
    static int calculate_signal_charge(float station_freq, float vfo_freq);

    unsigned long _frequency;
    byte _sub_frequency;
    unsigned long _step;
    RealizationPool *_realization_pool;

private:
};

#endif // __VFO_H__
