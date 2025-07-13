#include "basic_types.h"
#include "wave_gen_pool.h"

// pass array of wave generator addresses, array of free/in-use bools, count of wave generators 
WaveGenPool::WaveGenPool(WaveGen **wavegens, bool *statuses,  int nwavegens){
    _realizers = wavegens;
    _statuses = statuses;
    _nrealizers = nwavegens;

    for(int i = 0; i < _nrealizers; i++){
        free_realizer(i, 0);  // Initialize with station_id 0
    }   
}

int WaveGenPool::get_realizer(int station_id){
    for(int i = 0; i < _nrealizers; i++){
        if(!_statuses[i]){
            _statuses[i] = true;
            return i;
        }
    }
    return -1;
}

// multiplely gotten realizers must be freed individually
void WaveGenPool::free_realizer(int nrealizer, int station_id){
    _statuses[nrealizer] = false;
}

WaveGen * WaveGenPool::access_realizer(int nrealizer){
    return _realizers[nrealizer];
}

int WaveGenPool::get_available_count(){
    int available = 0;
    for(int i = 0; i < _nrealizers; i++){
        if(!_statuses[i]){
            available++;
        }
    }
    return available;
}
