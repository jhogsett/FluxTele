#include "basic_types.h"
#include "realization.h"
#include "realization_pool.h"

// pass array of realizer addresses, array of free/in-use bools, count of realizers 
RealizationPool::RealizationPool(Realization **realizations, bool *statuses,  int nrealizations){
    _realizations = realizations;
    _statuses = statuses;
    _nrealizations = nrealizations; 
    _hardware_dirty = false;  // Initialize as clean
}

bool RealizationPool::begin(unsigned long time){
    return true;
}

bool RealizationPool::step(unsigned long time){
    for(byte i = 0; i < _nrealizations; i++){
        if(!_realizations[i]->step(time))
            return false;
    }
    return true;
}

void RealizationPool::end(){
}

void RealizationPool::update(Mode *mode){
    for(byte i = 0; i < _nrealizations; i++){
        _realizations[i]->update(mode);
    }
    
    // If hardware state is dirty (unknown), force a refresh
    if(_hardware_dirty) {
        force_sim_transmitter_refresh();
        _hardware_dirty = false;  // Clear the dirty flag
    }
}

// JH! This method ought to be renamed, because RealizationPool _should_ have knowledge of _Realization_ but _SHOULD NOT_ be aware of its derived types
void RealizationPool::force_sim_transmitter_refresh(){
// JH!

    // Force wave generator hardware refresh for all SimTransmitter objects
    // This is called when switching to SimRadio to ensure wave generators
    // are properly synchronized with their software state
    for(byte i = 0; i < _nrealizations; i++){
        // Use virtual method instead of dynamic_cast for Arduino compatibility
        _realizations[i]->force_wave_generator_refresh();
    }
}

// JH! The word "dirty" is a bit uncomfortable. This should be renamed with a term that means something like "needs refresh" or "invalidate hardware"
void RealizationPool::mark_dirty(){
    // Mark hardware state as unknown - will trigger refresh on next update
    // This is called when switching applications that may affect hardware state
    _hardware_dirty = true;
}
// JH!
