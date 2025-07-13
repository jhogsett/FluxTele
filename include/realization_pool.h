#ifndef __REALIZATION_POOL_H__
#define __REALIZATION_POOL_H__

#include "mode.h"
#include "realization.h"

// initialize with an array of realizers
// tracks whether they are in use
// can request 1 or more realizers

class RealizationPool
{
public:
    // pass array of realizer addresses, array of free/in-use bools, count of realizers 
    RealizationPool(Realization **realizations, bool *statuses,  int nrealizations);

    bool begin(unsigned long time);
    bool step(unsigned long time);
    void end();

    void update(Mode *mode);
    void force_sim_transmitter_refresh();  // Force hardware refresh for SimTransmitter objects
    void mark_dirty();  // Mark hardware state as unknown - triggers refresh on next update

private:
    Realization **_realizations;
    bool *_statuses;
    int _nrealizations;
    bool _hardware_dirty;  // True when hardware state is unknown and needs refresh
};

#endif // __REALIZER_POOL_H__
