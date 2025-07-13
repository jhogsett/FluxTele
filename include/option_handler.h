#ifndef __OPTION_HANDLER_H__
#define __OPTION_HANDLER_H__

#include <HT16K33Disp.h>
#include "mode.h"
#include "mode_handler.h"

class Option_Handler : public ModeHandler
{
public:
    // constructor
    Option_Handler(Mode * mode);

    virtual bool event_sink(int event, int count);
    virtual bool event_sink(bool pressed, bool long_pressed);

    // virtual void step(unsigned long time);

    void next_option();
    void prev_option();

private:
};

#endif
