#include "mode.h"
#include "wave_gen_pool.h"
#include "realization.h"

Realization::Realization(WaveGenPool *wave_gen_pool, int station_id, int required_realizers){
    _wave_gen_pool = wave_gen_pool;
    _station_id = station_id;
    _required_realizers = required_realizers;
    
    // Validate required_realizers count
    if(_required_realizers < 1) _required_realizers = 1;
    if(_required_realizers > MAX_REALIZERS_PER_STATION) _required_realizers = MAX_REALIZERS_PER_STATION;
    
    // Initialize all realizer slots to -1 (not acquired)
    for(int i = 0; i < MAX_REALIZERS_PER_STATION; i++) {
        _realizers[i] = -1;
    }
    
    // Legacy compatibility
    _realizer = -1;
}

// returns true on successful update
bool Realization::update(Mode *mode){
    return false;
}

// returns true on successful begin - acquires ALL required realizers atomically
bool Realization::begin(unsigned long time){
    // If already have all realizers, begin() is idempotent - just return success
    if(has_all_realizers()) {
        return true;
    }
    
    // Attempt to acquire all required realizers atomically
    int temp_realizers[MAX_REALIZERS_PER_STATION];
    for(int i = 0; i < MAX_REALIZERS_PER_STATION; i++) {
        temp_realizers[i] = -1;
    }
    
    // Try to acquire all needed realizers
    for(int i = 0; i < _required_realizers; i++) {
        temp_realizers[i] = _wave_gen_pool->get_realizer(_station_id);
        if(temp_realizers[i] == -1) {
            // Failed to get all realizers - free any we got and return false
            for(int j = 0; j < i; j++) {
                _wave_gen_pool->free_realizer(temp_realizers[j], _station_id);
            }
            return false;
        }
    }
    
    // Success - copy temp array to actual array
    for(int i = 0; i < _required_realizers; i++) {
        _realizers[i] = temp_realizers[i];
    }
    
    // Legacy compatibility - set _realizer to first acquired realizer
    _realizer = _realizers[0];
    
    return true;
}

// call periodically to keep realization dynamic
// returns true if it should keep going
bool Realization::step(unsigned long time){
    // if(time >= _next_internal_step){
    //     _next_internal_step = time + _period;
    //     internal_step(time);
    // }
    return true;
}

// void Realization::internal_step(unsigned long time){
// }

void Realization::end(){
    // Free all acquired realizers
    for(int i = 0; i < _required_realizers; i++) {
        if(_realizers[i] != -1) {
            _wave_gen_pool->free_realizer(_realizers[i], _station_id);
            _realizers[i] = -1;
        }
    }
    
    // Legacy compatibility
    _realizer = -1;
}

// Get specific realizer by index (0-based)
int Realization::get_realizer(int index) const {
    if(index < 0 || index >= _required_realizers) {
        return -1;  // Invalid index
    }
    return _realizers[index];
}

// Check if all required realizers are acquired
bool Realization::has_all_realizers() const {
    for(int i = 0; i < _required_realizers; i++) {
        if(_realizers[i] == -1) {
            return false;
        }
    }
    return true;
}
