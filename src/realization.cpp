#include "mode.h"
#include "wave_gen_pool.h"
#include "realization.h"

Realization::Realization(WaveGenPool *wave_gen_pool, int station_id){
    _wave_gen_pool = wave_gen_pool;
    _realizer = -1;
    _station_id = station_id;
}

// returns true on successful update
bool Realization::update(Mode *mode){
    return false;
}

// returns true on successful begin
bool Realization::begin(unsigned long time){
    // If already have a realizer, begin() is idempotent - just return success
    if(_realizer != -1) {
        return true;
    }
    
    // attempt to acquire a realizer
    _realizer = _wave_gen_pool->get_realizer(_station_id);
    if(_realizer == -1)
        return false;
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
    if(_realizer != -1) {
        _wave_gen_pool->free_realizer(_realizer, _station_id);
        _realizer = -1;  // Reset to avoid double-free or invalid access
    }
}
