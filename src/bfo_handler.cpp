#include "bfo.h"
#include "bfo_handler.h"
#include "saved_data.h"

BFOHandler::BFOHandler(Mode * mode) : ModeHandler(mode) 
{
}

// does mode-specific handling of the event to modify the mode
// returns true if event was consumed
bool BFOHandler::event_sink(int event, int event_data){
    BFO *bfo = (BFO*) _mode;

    if(event == 1){
        bfo->next_option();
    } else if(event == -1){
        bfo->prev_option();
    }

    save_data();

    return true;
}

// JH! This just returns false, and also the same is seen in ContrastHandler and OptionHandler, so maybe there should be a common base class with this functionality
bool BFOHandler::event_sink(bool pressed, bool long_pressed){
    return false;
}
