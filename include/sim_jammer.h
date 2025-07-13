#ifndef __SIM_JAMMER_H__
#define __SIM_JAMMER_H__

#include "async_jammer.h"
#include "sim_transmitter.h"

class SimJammer : public SimTransmitter
{
public:
    SimJammer(WaveGenPool *wave_gen_pool);
    
    virtual bool begin(unsigned long time, float fixed_freq);
    virtual bool update(Mode *mode);
    virtual bool step(unsigned long time);
    virtual void realize();
    
private:
    AsyncJammer _jammer;
};

#endif
