#include "vfo.h"
#include "vfo_tuner.h"

VFO_Tuner::VFO_Tuner(Mode * mode) : ModeHandler(mode)
{
}

// does mode-specific handling of the event to modify the mode
// returns true if event was consumed
bool VFO_Tuner::event_sink(int event, int event_data){
    VFO *vfo = (VFO*) _mode;

    unsigned long _old_freq = vfo->_frequency;
    if(event == 1){
        vfo->_frequency += vfo->_step;
        if(_old_freq > vfo->_frequency){
            // unsigned long wrapped up??
            vfo->_frequency = (unsigned long)-1L;
        }        
    } else if(event == -1){
        vfo->_frequency -= vfo->_step;
        if(_old_freq < vfo->_frequency){
            // unsigned long wrapped down??
            vfo->_frequency = 0;
        }
    }

    return true;
}

bool VFO_Tuner::event_sink(bool pressed, bool long_pressed){
    return false;
}

