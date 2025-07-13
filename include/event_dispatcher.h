#ifndef __EVENT_DISPATCHER_H__
#define __EVENT_DISPATCHER_H__

#include "mode_handler.h"
#include "realization.h"

// Forward declaration
class SignalMeter;

#define ID_ENCODER_TUNING 0
#define ID_ENCODER_MODES 1

// input: encoder events
// has: current mode handler
// output: issues events to the current mode handler

class EventDispatcher
{
public:
    EventDispatcher(ModeHandler **mode_handlers, int nhandlers);

    void set_mode(int nhandler);
    void set_mode(HT16K33Disp *display, int nhandler);
    
    // Non-blocking title display management
    bool step_title_display(HT16K33Disp *display);
    bool is_showing_title() const;
     
    bool dispatch_event(HT16K33Disp *display, int encoder_id, int event, int event_data);
    bool dispatch_event(HT16K33Disp *display, int encoder_id, bool press, bool long_press);

    void update_display(HT16K33Disp *display);
    void update_signal_meter(SignalMeter *signal_meter);
    void update_realization();
    
    Mode* get_current_mode();  // Access current mode for refresh operations

private:
    ModeHandler * _mode_handler;
    ModeHandler ** _mode_handlers;
    int _nhandlers;
    int _ncurrent_handler;
    
    // Non-blocking title display state
    bool _showing_title;
    bool _pending_display_update;
    bool _pending_realization_update;
};

#endif // __EVENT_DISPATCHER_H__
