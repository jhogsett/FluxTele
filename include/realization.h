#ifndef __REALIZATION_H__
#define __REALIZATION_H__

#include "mode.h"
#include "wave_gen_pool.h"

// handles realization using a realizer
// 

class Mode;

class Realization
{
public:
    Realization(WaveGenPool *wave_gen_pool, int station_id = 0);

    virtual bool update(Mode *mode);

    virtual bool begin(unsigned long time);
    virtual bool step(unsigned long time);
    virtual void end();
    
    // Update station ID for debugging (used by jammer which sets frequency dynamically)
    void set_station_id(int station_id) { _station_id = station_id; }
    
    // Virtual method for wave generator refresh - default does nothing
    virtual void force_wave_generator_refresh() {}

    WaveGenPool *_wave_gen_pool;
    int _realizer;
    int _station_id;
};

#endif
