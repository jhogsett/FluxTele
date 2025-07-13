#ifndef __MODE_HANDLER_H__
#define __MODE_HANDLER_H__

#include <HT16K33Disp.h>
#include "mode.h"
#include "realization.h"

// Forward declaration
class SignalMeter;

// generic mode handler, associated to a mode, has mode-specific methods for operating on a mode
// modes: vfo, options
// associated to a mode realizer


// has: mode

class ModeHandler
{
public:
    ModeHandler(Mode *mode);

    void set_mode(Mode *mode);

    virtual bool event_sink(int event, int event_data);
    virtual bool event_sink(bool pressed, bool long_pressed);

    // virtual void step(unsigned long time);

    void show_title(HT16K33Disp *display);
    bool begin_show_title(HT16K33Disp *display);  // Non-blocking version
    bool step_show_title(HT16K33Disp *display);   // Non-blocking step
    virtual void update_display(HT16K33Disp *display);
    virtual void update_signal_meter(SignalMeter *signal_meter);
    virtual void update_realization();
    
    Mode* get_mode() { return _mode; }  // Access the current mode

protected:
    Mode *_mode;

};

#endif // __MODE_HANDLER_H__
