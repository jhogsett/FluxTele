#include "mode_handler.h"
#include "event_dispatcher.h"
#include "signal_meter.h"

// change to accept an array of mode handlers
EventDispatcher::EventDispatcher(ModeHandler **mode_handlers, int nhandlers){
    _mode_handlers = mode_handlers;
    _nhandlers = nhandlers;
    _mode_handler = NULL;
    _ncurrent_handler = 0;
    _showing_title = false;
    _pending_display_update = false;
    _pending_realization_update = false;
    set_mode(0);
}

void EventDispatcher::set_mode(int nhandler){
    _ncurrent_handler = nhandler;
    _mode_handler = _mode_handlers[_ncurrent_handler];
}

void EventDispatcher::set_mode(HT16K33Disp *display, int nhandler){
    set_mode(nhandler);
    // Start non-blocking title display
    _showing_title = _mode_handler->begin_show_title(display);
    _pending_display_update = true;
    _pending_realization_update = true;
}

//returns true if the event was consumed
// some events are meta-events, for example to change modes
bool EventDispatcher::dispatch_event(HT16K33Disp *display, int encoder_id, int event, int event_data){
    switch(encoder_id){
        case ID_ENCODER_TUNING:
            return _mode_handler->event_sink(event, event_data);

        case ID_ENCODER_MODES:
        {
            if(event == 1){
                int handler = _ncurrent_handler + 1;
                if(handler >= _nhandlers)
                handler = 0; 
                set_mode(display, handler);
                return true;
            } else if(event == -1){
                int handler = _ncurrent_handler - 1;
                if(handler < 0)
                handler = _nhandlers - 1; 
                set_mode(display, handler);
                return true;
            }
            break;
        }
    }
    return false;
}

bool EventDispatcher::dispatch_event(HT16K33Disp *display, int encoder_id, bool press, bool long_press){
    switch(encoder_id){
        case ID_ENCODER_TUNING:
            return _mode_handler->event_sink(press, long_press);

        case ID_ENCODER_MODES:
            return _mode_handler->event_sink(press, long_press);
    }
    return false;
}


void EventDispatcher::update_display(HT16K33Disp *display){
    _mode_handler->update_display(display);
}

void EventDispatcher::update_signal_meter(SignalMeter *signal_meter){
    _mode_handler->update_signal_meter(signal_meter);
}

void EventDispatcher::update_realization(){
    _mode_handler->update_realization();
}

Mode* EventDispatcher::get_current_mode(){
    return _mode_handler->get_mode();
}

// Non-blocking title display management
bool EventDispatcher::step_title_display(HT16K33Disp *display) {
    if (!_showing_title) {
        return false;
    }
    
    _showing_title = _mode_handler->step_show_title(display);
    
    // If title display just finished, handle pending updates
    if (!_showing_title) {
        if (_pending_display_update) {
            update_display(display);
            _pending_display_update = false;
        }
        if (_pending_realization_update) {
            update_realization();
            _pending_realization_update = false;
        }
    }
    
    return _showing_title;
}

bool EventDispatcher::is_showing_title() const {
    return _showing_title;
}
