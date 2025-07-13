#ifndef __WAVEGEN_POOL_H__
#define __WAVEGEN_POOL_H__

#include "wavegen.h"

// initialize with an array of wave generators
// tracks whether they are in use
// can request 1 or more wave generators for station audio output

class WaveGenPool
{
public:
    // pass array of wave generator addresses, array of free/in-use bools, count of wave generators 
    WaveGenPool(WaveGen **wavegens, bool *statuses,  int nwavegens);

    // returns -1 if not available otherwise wave generator index into array
    int get_realizer(int station_id = 0);

    // multiplely gotten wave generators must be freed individually
    void free_realizer(int nrealizer, int station_id = 0);

    WaveGen * access_realizer(int nrealizer);

    // Get resource statistics for debugging
    int get_available_count();
    int get_total_count() { return _nrealizers; }

private:
    WaveGen **_realizers;
    bool *_statuses;
    int _nrealizers;

};

#endif // __WAVEGEN_POOL_H__
