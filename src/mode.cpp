#include <HT16K33Disp.h>
#include "mode.h"
#include "signal_meter.h"

Mode::Mode(const char *title){
    _title = title;
}

void Mode::update_display(HT16K33Disp *display){

}

void Mode::update_signal_meter(SignalMeter *signal_meter){
    // Default implementation - clear signal meter
    signal_meter->update_signal_strength(0);
}

void Mode::update_realization(){
    
}
