#ifndef __MODE_H__
#define __MODE_H__

#include <HT16K33Disp.h>
#include "realization.h"

// Forward declaration
class SignalMeter;

// generic mode, has data and methods specific to a mode

// a mode has public methods/data
// modified by a mode handler
// has a method to update the display


class Mode
{
public:
    Mode(const char *title);

    virtual void update_display(HT16K33Disp *display);
    virtual void update_signal_meter(SignalMeter *signal_meter);
    virtual void update_realization();

    const char *_title;

private:
};

#endif // __MODE_H__
